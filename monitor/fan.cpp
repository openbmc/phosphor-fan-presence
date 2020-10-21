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

    // Get the initial presence state
    _present = util::SDBusPlus::getProperty<bool>(
        util::INVENTORY_PATH + _name, util::INV_ITEM_IFACE, "Present");

    if (_fanMissingErrorDelay)
    {
        _fanMissingErrorTimer = std::make_unique<
            sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>>(
            event, std::bind(&System::fanMissingErrorTimerExpired, &system,
                             std::ref(*this)));

        if (!_present)
        {
            // The fan presence application handles the journal for missing
            // fans, so only internally log missing fan info here.
            getLogger().log(fmt::format("On startup, fan {} is missing", _name),
                            Logger::quiet);
            _fanMissingErrorTimer->restartOnce(
                std::chrono::seconds{*_fanMissingErrorDelay});
        }
    }
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
                    sensor.stopTimer();
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

bool Fan::tooManySensorsNonfunctional()
{
    size_t numFailed =
        std::count_if(_sensors.begin(), _sensors.end(),
                      [](const auto& s) { return !s->functional(); });

    return (numFailed >= _numSensorFailsForNonFunc);
}

bool Fan::outOfRange(const TachSensor& sensor)
{
    auto actual = static_cast<uint64_t>(sensor.getInput());
    auto target = sensor.getTarget();
    auto factor = sensor.getFactor();
    auto offset = sensor.getOffset();

    uint64_t min = target * (100 - _deviation) / 100;
    uint64_t max = target * (100 + _deviation) / 100;

    // TODO: openbmc/openbmc#2937 enhance this function
    // either by making it virtual, or by predefining different
    // outOfRange ops and selecting by yaml config
    min = min * factor + offset;
    max = max * factor + offset;
    if ((actual < min) || (actual > max))
    {
        return true;
    }

    return false;
}

void Fan::updateState(TachSensor& sensor)
{
    sensor.setFunctional(!sensor.functional());

    getLogger().log(
        fmt::format("Setting tach sensor {} functional state to {}. "
                    "Actual speed: {} Target speed: {}",
                    sensor.name(), sensor.functional(), sensor.getInput(),
                    sensor.getTarget()));

    // A zero value for _numSensorFailsForNonFunc means we aren't dealing
    // with fan FRU functional status, only sensor functional status.
    if (_numSensorFailsForNonFunc)
    {
        // If the fan was nonfunctional and enough sensors are now OK,
        // the fan can go back to functional
        if (!_functional && !tooManySensorsNonfunctional())
        {
            getLogger().log(
                fmt::format("Setting fan {} back to functional", _name));

            updateInventory(true);
        }

        // If the fan is currently functional, but too many
        // contained sensors are now nonfunctional, update
        // the whole fan nonfunctional.
        if (_functional && tooManySensorsNonfunctional())
        {
            getLogger().log(fmt::format("Setting fan {} to nonfunctional "
                                        "Sensor: {} "
                                        "Actual speed: {} "
                                        "Target speed: {}",
                                        _name, sensor.name(), sensor.getInput(),
                                        sensor.getTarget()));
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
            fmt::format("Fan {} presence state change to {}", _name, _present),
            Logger::quiet);

        _system.fanStatusChange(*this);

        if (_fanMissingErrorDelay)
        {
            if (!_present)
            {
                _fanMissingErrorTimer->restartOnce(
                    std::chrono::seconds{*_fanMissingErrorDelay});
            }
            else if (_fanMissingErrorTimer->isEnabled())
            {
                _fanMissingErrorTimer->setEnabled(false);
            }
        }
    }
}

void Fan::sensorErrorTimerExpired(const TachSensor& sensor)
{
    if (_present)
    {
        _system.sensorErrorTimerExpired(*this, sensor);
    }
}

} // namespace monitor
} // namespace fan
} // namespace phosphor
