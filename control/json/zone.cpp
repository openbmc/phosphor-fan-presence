/**
 * Copyright © 2022 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "zone.hpp"

#include "../utils/flight_recorder.hpp"
#include "dbus_zone.hpp"
#include "fan.hpp"
#include "sdbusplus.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdeventplus/event.hpp>

#include <algorithm>
#include <chrono>
#include <iterator>
#include <map>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using namespace phosphor::logging;

const std::map<
    std::string,
    std::map<std::string, std::function<std::function<void(DBusZone&, Zone&)>(
                              const json&, bool)>>>
    Zone::_intfPropHandlers = {
        {DBusZone::thermalModeIntf,
         {{DBusZone::supportedProp, zone::property::supported},
          {DBusZone::currentProp, zone::property::current}}}};

Zone::Zone(const json& jsonObj, const sdeventplus::Event& event, Manager* mgr) :
    ConfigBase(jsonObj), _dbusZone{}, _manager(mgr), _defaultFloor(0),
    _incDelay(0), _decInterval(0), _floor(0), _target(0), _incDelta(0),
    _decDelta(0), _requestTargetBase(0), _isActive(true),
    _incTimer(event, std::bind(&Zone::incTimerExpired, this)),
    _decTimer(event, std::bind(&Zone::decTimerExpired, this))
{
    // Increase delay is optional, defaults to 0
    if (jsonObj.contains("increase_delay"))
    {
        _incDelay =
            std::chrono::seconds(jsonObj["increase_delay"].get<uint64_t>());
    }

    // Poweron target is required
    setPowerOnTarget(jsonObj);

    // Default ceiling is optional, defaults to poweron target
    _defaultCeiling = _poweronTarget;
    if (jsonObj.contains("default_ceiling"))
    {
        _defaultCeiling = jsonObj["default_ceiling"].get<uint64_t>();
    }
    // Start with the current ceiling set as the default ceiling
    _ceiling = _defaultCeiling;

    // Default floor is optional, defaults to 0
    if (jsonObj.contains("default_floor"))
    {
        _defaultFloor = jsonObj["default_floor"].get<uint64_t>();
        if (_defaultFloor > _ceiling)
        {
            log<level::ERR>(
                fmt::format("Configured default_floor({}) above ceiling({}), "
                            "setting default floor to ceiling",
                            _defaultFloor, _ceiling)
                    .c_str());
            _defaultFloor = _ceiling;
        }
        // Start with the current floor set as the default
        _floor = _defaultFloor;
    }

    // Decrease interval is optional, defaults to 0
    // A decrease interval of 0sec disables the decrease timer
    if (jsonObj.contains("decrease_interval"))
    {
        _decInterval =
            std::chrono::seconds(jsonObj["decrease_interval"].get<uint64_t>());
    }

    // Setting properties on interfaces to be served are optional
    if (jsonObj.contains("interfaces"))
    {
        setInterfaces(jsonObj);
    }
}

void Zone::enable()
{
    // Create thermal control dbus object
    _dbusZone = std::make_unique<DBusZone>(*this);

    // Init all configured dbus interfaces' property states
    for (const auto& func : _propInitFunctions)
    {
        // Only call non-null init property functions
        if (func)
        {
            func(*_dbusZone, *this);
        }
    }

    // TODO - Restore any persisted properties in init function
    // Restore thermal control current mode state, if exists
    _dbusZone->restoreCurrentMode();

    // Emit object added for this zone's associated dbus object
    _dbusZone->emit_object_added();

    // A decrease interval of 0sec disables the decrease timer
    if (_decInterval != std::chrono::seconds::zero())
    {
        // Start timer for fan target decreases
        _decTimer.restart(_decInterval);
    }
}

void Zone::addFan(std::unique_ptr<Fan> fan)
{
    _fans.emplace_back(std::move(fan));
}

void Zone::setTarget(uint64_t target)
{
    if (_isActive)
    {
        if (_target != target)
        {
            FlightRecorder::instance().log(
                "zone-set-target" + getName(),
                fmt::format("Set target {} (from {})", target, _target));
        }
        _target = target;
        for (auto& fan : _fans)
        {
            fan->setTarget(_target);
        }
    }
}

void Zone::lockFanTarget(const std::string& fname, uint64_t target)
{
    auto fanItr =
        std::find_if(_fans.begin(), _fans.end(), [&fname](const auto& fan) {
            return fan->getName() == fname;
        });

    if (_fans.end() != fanItr)
    {
        (*fanItr)->lockTarget(target);
    }
    else
    {
        log<level::DEBUG>(
            fmt::format("Configured fan {} not found in zone {} to lock target",
                        fname, getName())
                .c_str());
    }
}

void Zone::unlockFanTarget(const std::string& fname, uint64_t target)
{
    auto fanItr =
        std::find_if(_fans.begin(), _fans.end(), [&fname](const auto& fan) {
            return fan->getName() == fname;
        });

    if (_fans.end() != fanItr)
    {
        (*fanItr)->unlockTarget(target);

        // attempt to resume Zone target on fan
        (*fanItr)->setTarget(getTarget());
    }
    else
    {
        log<level::DEBUG>(
            fmt::format(
                "Configured fan {} not found in zone {} to unlock target",
                fname, getName())
                .c_str());
    }
}

void Zone::setTargetHold(const std::string& ident, uint64_t target, bool hold)
{
    using namespace std::string_literals;

    if (!hold)
    {
        size_t removed = _targetHolds.erase(ident);
        if (removed)
        {
            FlightRecorder::instance().log(
                "zone-target"s + getName(),
                fmt::format("{} is removing target hold", ident));
        }
    }
    else
    {
        if (!((_targetHolds.find(ident) != _targetHolds.end()) &&
              (_targetHolds[ident] == target)))
        {
            FlightRecorder::instance().log(
                "zone-target"s + getName(),
                fmt::format("{} is setting target hold to {}", ident, target));
        }
        _targetHolds[ident] = target;
        _isActive = false;
    }

    auto itHoldMax = std::max_element(_targetHolds.begin(), _targetHolds.end(),
                                      [](const auto& aHold, const auto& bHold) {
                                          return aHold.second < bHold.second;
                                      });
    if (itHoldMax == _targetHolds.end())
    {
        _isActive = true;
    }
    else
    {
        if (_target != itHoldMax->second)
        {
            FlightRecorder::instance().log(
                "zone-target"s + getName(),
                fmt::format("Settings fans to target hold of {}",
                            itHoldMax->second));
        }

        _target = itHoldMax->second;
        for (auto& fan : _fans)
        {
            fan->setTarget(_target);
        }
    }
}

void Zone::setFloorHold(const std::string& ident, uint64_t target, bool hold)
{
    using namespace std::string_literals;

    if (target > _ceiling)
    {
        target = _ceiling;
    }

    if (!hold)
    {
        size_t removed = _floorHolds.erase(ident);
        if (removed)
        {
            FlightRecorder::instance().log(
                "zone-floor"s + getName(),
                fmt::format("{} is removing floor hold", ident));
        }
    }
    else
    {
        if (!((_floorHolds.find(ident) != _floorHolds.end()) &&
              (_floorHolds[ident] == target)))
        {
            FlightRecorder::instance().log(
                "zone-floor"s + getName(),
                fmt::format("{} is setting floor hold to {}", ident, target));
        }
        _floorHolds[ident] = target;
    }

    if (!std::all_of(_floorChange.begin(), _floorChange.end(),
                     [](const auto& entry) { return entry.second; }))
    {
        return;
    }

    auto itHoldMax = std::max_element(_floorHolds.begin(), _floorHolds.end(),
                                      [](const auto& aHold, const auto& bHold) {
                                          return aHold.second < bHold.second;
                                      });
    if (itHoldMax == _floorHolds.end())
    {
        if (_floor != _defaultFloor)
        {
            FlightRecorder::instance().log(
                "zone-floor"s + getName(),
                fmt::format("No set floor exists, using default floor",
                            _defaultFloor));
        }
        _floor = _defaultFloor;
    }
    else
    {
        if (_floor != itHoldMax->second)
        {
            FlightRecorder::instance().log(
                "zone-floor"s + getName(),
                fmt::format("Setting new floor to {}", itHoldMax->second));
        }
        _floor = itHoldMax->second;
    }

    // Floor above target, update target to floor
    if (_target < _floor)
    {
        requestIncrease(_floor - _target);
    }
}

void Zone::setFloor(uint64_t target)
{
    // Check all entries are set to allow floor to be set
    auto pred = [](const auto& entry) { return entry.second; };
    if (std::all_of(_floorChange.begin(), _floorChange.end(), pred))
    {
        _floor = (target > _ceiling) ? _ceiling : target;
        // Floor above target, update target to floor
        if (_target < _floor)
        {
            requestIncrease(_floor - _target);
        }
    }
}

void Zone::requestIncrease(uint64_t targetDelta)
{
    // Only increase when delta is higher than the current increase delta for
    // the zone and currently under ceiling
    if (targetDelta > _incDelta && _target < _ceiling)
    {
        auto requestTarget = getRequestTargetBase();
        requestTarget = (targetDelta - _incDelta) + requestTarget;
        _incDelta = targetDelta;
        // Target can not go above a current ceiling
        if (requestTarget > _ceiling)
        {
            requestTarget = _ceiling;
        }
        setTarget(requestTarget);
        // Restart timer countdown for fan target increase
        _incTimer.restartOnce(_incDelay);
    }
}

void Zone::incTimerExpired()
{
    // Clear increase delta when timer expires allowing additional target
    // increase requests or target decreases to occur
    _incDelta = 0;
}

void Zone::requestDecrease(uint64_t targetDelta)
{
    // Only decrease the lowest target delta requested
    if (_decDelta == 0 || targetDelta < _decDelta)
    {
        _decDelta = targetDelta;
    }
}

void Zone::decTimerExpired()
{
    // Check all entries are set to allow a decrease
    auto pred = [](auto const& entry) { return entry.second; };
    auto decAllowed = std::all_of(_decAllowed.begin(), _decAllowed.end(), pred);

    // Only decrease targets when allowed, a requested decrease target delta
    // exists, where no requested increases exist and the increase timer is not
    // running (i.e. not in the middle of increasing)
    if (decAllowed && _decDelta != 0 && _incDelta == 0 &&
        !_incTimer.isEnabled())
    {
        auto requestTarget = getRequestTargetBase();
        // Request target should not start above ceiling
        if (requestTarget > _ceiling)
        {
            requestTarget = _ceiling;
        }
        // Target can not go below the defined floor
        if ((requestTarget < _decDelta) || (requestTarget - _decDelta < _floor))
        {
            requestTarget = _floor;
        }
        else
        {
            requestTarget = requestTarget - _decDelta;
        }
        setTarget(requestTarget);
    }
    // Clear decrease delta when timer expires
    _decDelta = 0;
    // Decrease timer is restarted since its repeating
}

void Zone::setPersisted(const std::string& intf, const std::string& prop)
{
    if (std::find_if(_propsPersisted[intf].begin(), _propsPersisted[intf].end(),
                     [&prop](const auto& p) { return prop == p; }) ==
        _propsPersisted[intf].end())
    {
        _propsPersisted[intf].emplace_back(prop);
    }
}

bool Zone::isPersisted(const std::string& intf, const std::string& prop) const
{
    auto it = _propsPersisted.find(intf);
    if (it == _propsPersisted.end())
    {
        return false;
    }

    return std::any_of(it->second.begin(), it->second.end(),
                       [&prop](const auto& p) { return prop == p; });
}

void Zone::setPowerOnTarget(const json& jsonObj)
{
    if (!jsonObj.contains("poweron_target"))
    {
        auto msg = "Missing required zone's poweron target";
        log<level::ERR>(msg, entry("JSON=%s", jsonObj.dump().c_str()));
        throw std::runtime_error(msg);
    }
    _poweronTarget = jsonObj["poweron_target"].get<uint64_t>();
}

void Zone::setInterfaces(const json& jsonObj)
{
    for (const auto& interface : jsonObj["interfaces"])
    {
        if (!interface.contains("name") || !interface.contains("properties"))
        {
            log<level::ERR>("Missing required zone interface attributes",
                            entry("JSON=%s", interface.dump().c_str()));
            throw std::runtime_error(
                "Missing required zone interface attributes");
        }
        auto propFuncs =
            _intfPropHandlers.find(interface["name"].get<std::string>());
        if (propFuncs == _intfPropHandlers.end())
        {
            // Construct list of available configurable interfaces
            auto intfs = std::accumulate(
                std::next(_intfPropHandlers.begin()), _intfPropHandlers.end(),
                _intfPropHandlers.begin()->first, [](auto list, auto intf) {
                    return std::move(list) + ", " + intf.first;
                });
            log<level::ERR>("Configured interface not available",
                            entry("JSON=%s", interface.dump().c_str()),
                            entry("AVAILABLE_INTFS=%s", intfs.c_str()));
            throw std::runtime_error("Configured interface not available");
        }

        for (const auto& property : interface["properties"])
        {
            if (!property.contains("name"))
            {
                log<level::ERR>(
                    "Missing required interface property attributes",
                    entry("JSON=%s", property.dump().c_str()));
                throw std::runtime_error(
                    "Missing required interface property attributes");
            }
            // Attribute "persist" is optional, defaults to `false`
            auto persist = false;
            if (property.contains("persist"))
            {
                persist = property["persist"].get<bool>();
            }
            // Property name from JSON must exactly match supported
            // index names to functions in property namespace
            auto propFunc =
                propFuncs->second.find(property["name"].get<std::string>());
            if (propFunc == propFuncs->second.end())
            {
                // Construct list of available configurable properties
                auto props = std::accumulate(
                    std::next(propFuncs->second.begin()),
                    propFuncs->second.end(), propFuncs->second.begin()->first,
                    [](auto list, auto prop) {
                        return std::move(list) + ", " + prop.first;
                    });
                log<level::ERR>("Configured property not available",
                                entry("JSON=%s", property.dump().c_str()),
                                entry("AVAILABLE_PROPS=%s", props.c_str()));
                throw std::runtime_error(
                    "Configured property function not available");
            }

            _propInitFunctions.emplace_back(
                propFunc->second(property, persist));
        }
    }
}

json Zone::dump() const
{
    json output;

    output["active"] = _isActive;
    output["floor"] = _floor;
    output["ceiling"] = _ceiling;
    output["target"] = _target;
    output["increase_delta"] = _incDelta;
    output["decrease_delta"] = _decDelta;
    output["power_on_target"] = _poweronTarget;
    output["default_ceiling"] = _defaultCeiling;
    output["default_floor"] = _defaultFloor;
    output["increase_delay"] = _incDelay.count();
    output["decrease_interval"] = _decInterval.count();
    output["requested_target_base"] = _requestTargetBase;
    output["floor_change"] = _floorChange;
    output["decrease_allowed"] = _decAllowed;
    output["persisted_props"] = _propsPersisted;
    output["target_holds"] = _targetHolds;
    output["floor_holds"] = _floorHolds;

    std::map<std::string, std::vector<uint64_t>> lockedTargets;
    for (const auto& fan : _fans)
    {
        const auto& locks = fan->getLockedTargets();
        if (!locks.empty())
        {
            lockedTargets[fan->getName()] = locks;
        }
    }
    output["target_locks"] = lockedTargets;

    return output;
}

/**
 * Properties of interfaces supported by the zone configuration that return
 * a handler function that sets the zone's property value(s) and persist
 * state.
 */
namespace zone::property
{
// Get a set property handler function for the configured values of the
// "Supported" property
std::function<void(DBusZone&, Zone&)> supported(const json& jsonObj,
                                                bool persist)
{
    std::vector<std::string> values;
    if (!jsonObj.contains("values"))
    {
        log<level::ERR>("No 'values' found for \"Supported\" property, "
                        "using an empty list",
                        entry("JSON=%s", jsonObj.dump().c_str()));
    }
    else
    {
        for (const auto& value : jsonObj["values"])
        {
            if (!value.contains("value"))
            {
                log<level::ERR>("No 'value' found for \"Supported\" property "
                                "entry, skipping",
                                entry("JSON=%s", value.dump().c_str()));
            }
            else
            {
                values.emplace_back(value["value"].get<std::string>());
            }
        }
    }

    return Zone::setProperty<std::vector<std::string>>(
        DBusZone::thermalModeIntf, DBusZone::supportedProp,
        &DBusZone::supported, std::move(values), persist);
}

// Get a set property handler function for a configured value of the
// "Current" property
std::function<void(DBusZone&, Zone&)> current(const json& jsonObj, bool persist)
{
    // Use default value for "Current" property if no "value" entry given
    if (!jsonObj.contains("value"))
    {
        log<level::INFO>("No 'value' found for \"Current\" property, "
                         "using default",
                         entry("JSON=%s", jsonObj.dump().c_str()));
        // Set persist state of property
        return Zone::setPropertyPersist(DBusZone::thermalModeIntf,
                                        DBusZone::currentProp, persist);
    }

    return Zone::setProperty<std::string>(
        DBusZone::thermalModeIntf, DBusZone::currentProp, &DBusZone::current,
        jsonObj["value"].get<std::string>(), persist);
}

} // namespace zone::property

} // namespace phosphor::fan::control::json
