/**
 * Copyright © 2017 IBM Corporation
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
#include "fan.hpp"

#include "logging.hpp"
#include "sdbusplus.hpp"
#include "system.hpp"
#include "types.hpp"
#include "utility.hpp"

#include <fmt/format.h>

#include <phosphor-logging/log.hpp>

#include <algorithm>

namespace phosphor
{
namespace fan
{
namespace monitor
{

using namespace phosphor::logging;
using namespace sdbusplus::bus::match;

Fan::Fan(Mode mode, sdbusplus::bus::bus& bus, const sdeventplus::Event& event,
         std::unique_ptr<trust::Manager>& trust, const FanDefinition& def,
         System& system) :
    _bus(bus),
    _name(std::get<fanNameField>(def)),
    _deviation(std::get<fanDeviationField>(def)),
    _numSensorFailsForNonFunc(std::get<numSensorFailsForNonfuncField>(def)),
    _trustManager(trust),
#ifdef MONITOR_USE_JSON
    _monitorDelay(std::get<monitorStartDelayField>(def)),
    _monitorTimer(event, std::bind(std::mem_fn(&Fan::startMonitor), this)),
#endif
    _system(system),
    _presenceMatch(bus,
                   rules::propertiesChanged(util::INVENTORY_PATH + _name,
                                            util::INV_ITEM_IFACE),
                   std::bind(std::mem_fn(&Fan::presenceChanged), this,
                             std::placeholders::_1)),
    _presenceIfaceAddedMatch(
        bus,
        rules::interfacesAdded() +
            rules::argNpath(0, util::INVENTORY_PATH + _name),
        std::bind(std::mem_fn(&Fan::presenceIfaceAdded), this,
                  std::placeholders::_1)),
    _fanMissingErrorDelay(std::get<fanMissingErrDelayField>(def))
{
    // Start from a known state of functional (even if
    // _numSensorFailsForNonFunc is 0)
    updateInventory(true);

    // Setup tach sensors for monitoring
    auto& sensors = std::get<sensorListField>(def);
    for (auto& s : sensors)
    {
        try
        {
            _sensors.emplace_back(std::make_shared<TachSensor>(
                mode, bus, *this, std::get<sensorNameField>(s),
                std::get<hasTargetField>(s), std::get<funcDelay>(def),
                std::get<targetInterfaceField>(s), std::get<factorField>(s),
                std::get<offsetField>(s), std::get<methodField>(def),
                std::get<thresholdField>(s), std::get<timeoutField>(def),
                std::get<nonfuncRotorErrDelayField>(def), event));

            _trustManager->registerSensor(_sensors.back());
        }
        catch (InvalidSensorError& e)
        {
            // Count the number of failed tach sensors, though if
            // _numSensorFailsForNonFunc is zero that means the fan should not
            // be set to nonfunctional.
            if (_numSensorFailsForNonFunc &&
                (++_numFailedSensor >= _numSensorFailsForNonFunc))
            {
                // Mark associated fan as nonfunctional
                updateInventory(false);
            }
        }
    }

#ifndef MONITOR_USE_JSON
    // Check current tach state when entering monitor mode
    if (mode != Mode::init)
    {
        _monitorReady = true;

        // The TachSensors will now have already read the input
        // and target values, so check them.
        tachChanged();
    }
#else
    // If it used the JSON config, then it also will do all the work
    // out of fan-monitor-init, after _monitorDelay.
    _monitorTimer.restartOnce(std::chrono::seconds(_monitorDelay));
#endif

    if (_fanMissingErrorDelay)
    {
        _fanMissingErrorTimer = std::make_unique<
            sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>>(
            event, std::bind(&System::fanMissingErrorTimerExpired, &system,
                             std::ref(*this)));
    }

    try
    {
        _present = util::SDBusPlus::getProperty<bool>(
            util::INVENTORY_PATH + _name, util::INV_ITEM_IFACE, "Present");

        if (!_present)
        {
            getLogger().log(
                fmt::format("On startup, fan {} is missing", _name));
            if (_system.isPowerOn() && _fanMissingErrorTimer)
            {
                _fanMissingErrorTimer->restartOnce(
                    std::chrono::seconds{*_fanMissingErrorDelay});
            }
        }
    }
    catch (const util::DBusServiceError& e)
    {
        // This could happen on the first BMC boot if the presence
        // detect app hasn't started yet and there isn't an inventory
        // cache yet.
    }
}

void Fan::presenceIfaceAdded(sdbusplus::message::message& msg)
{
    sdbusplus::message::object_path path;
    std::map<std::string, std::map<std::string, std::variant<bool>>> interfaces;

    msg.read(path, interfaces);

    auto properties = interfaces.find(util::INV_ITEM_IFACE);
    if (properties == interfaces.end())
    {
        return;
    }

    auto property = properties->second.find("Present");
    if (property == properties->second.end())
    {
        return;
    }

    _present = std::get<bool>(property->second);

    if (!_present)
    {
        getLogger().log(fmt::format(
            "New fan {} interface added and fan is not present", _name));
        if (_system.isPowerOn() && _fanMissingErrorTimer)
        {
            _fanMissingErrorTimer->restartOnce(
                std::chrono::seconds{*_fanMissingErrorDelay});
        }
    }

    _system.fanStatusChange(*this);
}

void Fan::startMonitor()
{
    _monitorReady = true;

    tachChanged();
}

void Fan::tachChanged()
{
    if (_monitorReady)
    {
        for (auto& s : _sensors)
        {
            tachChanged(*s);
        }
    }
}

void Fan::tachChanged(TachSensor& sensor)
{
    if (!_system.isPowerOn() || !_monitorReady)
    {
        return;
    }

    if (_trustManager->active())
    {
        if (!_trustManager->checkTrust(sensor))
        {
            return;
        }
    }

    // If this sensor is out of range at this moment, start
    // its timer, at the end of which the inventory
    // for the fan may get updated to not functional.

    // If this sensor is OK, put everything back into a good state.

    if (outOfRange(sensor))
    {
        if (sensor.functional())
        {
            switch (sensor.getMethod())
            {
                case MethodMode::timebased:
                    // Start nonfunctional timer if not already running
                    sensor.startTimer(TimerMode::nonfunc);
                    break;
                case MethodMode::count:
                    sensor.setCounter(true);
                    if (sensor.getCounter() >= sensor.getThreshold())
                    {
                        updateState(sensor);
                    }
                    break;
            }
        }
    }
    else
    {
        switch (sensor.getMethod())
        {
            case MethodMode::timebased:
                if (sensor.functional())
                {
                    if (sensor.timerRunning())
                    {
                        sensor.stopTimer();
                    }
                }
                else
                {
                    // Start functional timer if not already running
                    sensor.startTimer(TimerMode::func);
                }
                break;
            case MethodMode::count:
                sensor.setCounter(false);
                if (!sensor.functional() && sensor.getCounter() == 0)
                {
                    updateState(sensor);
                }
                break;
        }
    }
}

uint64_t Fan::findTargetSpeed()
{
    uint64_t target = 0;
    // The sensor doesn't support a target,
    // so get it from another sensor.
    auto s = std::find_if(_sensors.begin(), _sensors.end(),
                          [](const auto& s) { return s->hasTarget(); });

    if (s != _sensors.end())
    {
        target = (*s)->getTarget();
    }

    return target;
}

size_t Fan::countNonFunctionalSensors()
{
    return std::count_if(_sensors.begin(), _sensors.end(),
                         [](const auto& s) { return !s->functional(); });
}

bool Fan::outOfRange(const TachSensor& sensor)
{
    auto actual = static_cast<uint64_t>(sensor.getInput());
    auto range = sensor.getRange(_deviation);

    if ((actual < range.first) || (actual > range.second))
    {
        return true;
    }

    return false;
}

void Fan::updateState(TachSensor& sensor)
{
    auto range = sensor.getRange(_deviation);

    if (!_system.isPowerOn())
    {
        return;
    }

    sensor.setFunctional(!sensor.functional());
    getLogger().log(
        fmt::format("Setting tach sensor {} functional state to {}. "
                    "[target = {}, input = {}, allowed range = ({} - {})]",
                    sensor.name(), sensor.functional(), sensor.getTarget(),
                    sensor.getInput(), range.first, range.second));

    // A zero value for _numSensorFailsForNonFunc means we aren't dealing
    // with fan FRU functional status, only sensor functional status.
    if (_numSensorFailsForNonFunc)
    {
        auto numNonFuncSensors = countNonFunctionalSensors();
        // If the fan was nonfunctional and enough sensors are now OK,
        // the fan can be set to functional
        if (!_functional && !(numNonFuncSensors >= _numSensorFailsForNonFunc))
        {
            getLogger().log(fmt::format("Setting fan {} to functional, number "
                                        "of nonfunctional sensors = {}",
                                        _name, numNonFuncSensors));
            updateInventory(true);
        }

        // If the fan is currently functional, but too many
        // contained sensors are now nonfunctional, update
        // the fan to nonfunctional.
        if (_functional && (numNonFuncSensors >= _numSensorFailsForNonFunc))
        {
            getLogger().log(fmt::format("Setting fan {} to nonfunctional, "
                                        "number of nonfunctional sensors = {}",
                                        _name, numNonFuncSensors));
            updateInventory(false);
        }
    }

    _system.fanStatusChange(*this);
}

void Fan::updateInventory(bool functional)
{
    auto objectMap =
        util::getObjMap<bool>(_name, util::OPERATIONAL_STATUS_INTF,
                              util::FUNCTIONAL_PROPERTY, functional);
    auto response = util::SDBusPlus::lookupAndCallMethod(
        _bus, util::INVENTORY_PATH, util::INVENTORY_INTF, "Notify", objectMap);
    if (response.is_method_error())
    {
        log<level::ERR>("Error in Notify call to update inventory");
        return;
    }

    // This will always track the current state of the inventory.
    _functional = functional;
}

void Fan::presenceChanged(sdbusplus::message::message& msg)
{
    std::string interface;
    std::map<std::string, std::variant<bool>> properties;

    msg.read(interface, properties);

    auto presentProp = properties.find("Present");
    if (presentProp != properties.end())
    {
        _present = std::get<bool>(presentProp->second);

        getLogger().log(
            fmt::format("Fan {} presence state change to {}", _name, _present));

        _system.fanStatusChange(*this);

        if (_fanMissingErrorDelay)
        {
            if (!_present && _system.isPowerOn())
            {
                _fanMissingErrorTimer->restartOnce(
                    std::chrono::seconds{*_fanMissingErrorDelay});
            }
            else if (_present && _fanMissingErrorTimer->isEnabled())
            {
                _fanMissingErrorTimer->setEnabled(false);
            }
        }
    }
}

void Fan::sensorErrorTimerExpired(const TachSensor& sensor)
{
    if (_present && _system.isPowerOn())
    {
        _system.sensorErrorTimerExpired(*this, sensor);
    }
}

void Fan::powerStateChanged(bool powerStateOn)
{
#ifdef MONITOR_USE_JSON
    if (powerStateOn)
    {
        // set all fans back to functional to start with
        std::for_each(_sensors.begin(), _sensors.end(),
                      [](auto& sensor) { sensor->setFunctional(true); });

        _monitorTimer.restartOnce(std::chrono::seconds(_monitorDelay));

        if (!_present)
        {
            getLogger().log(
                fmt::format("At power on, fan {} is missing", _name));

            if (_fanMissingErrorTimer)
            {
                _fanMissingErrorTimer->restartOnce(
                    std::chrono::seconds{*_fanMissingErrorDelay});
            }
        }
    }
    else
    {
        _monitorReady = false;

        if (_monitorTimer.isEnabled())
        {
            _monitorTimer.setEnabled(false);
        }

        if (_fanMissingErrorTimer && _fanMissingErrorTimer->isEnabled())
        {
            _fanMissingErrorTimer->setEnabled(false);
        }

        std::for_each(_sensors.begin(), _sensors.end(), [](auto& sensor) {
            if (sensor->timerRunning())
            {
                sensor->stopTimer();
            }
        });
    }
#endif
}

} // namespace monitor
} // namespace fan
} // namespace phosphor
