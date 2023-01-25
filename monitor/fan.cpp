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
#include "fan.hpp"

#include "logging.hpp"
#include "sdbusplus.hpp"
#include "system.hpp"
#include "types.hpp"
#include "utility.hpp"

#include <fmt/format.h>

#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace fan
{
namespace monitor
{

using namespace phosphor::logging;
using namespace sdbusplus::bus::match;

Fan::Fan(Mode mode, sdbusplus::bus_t& bus, const sdeventplus::Event& event,
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
    _fanMissingErrorDelay(std::get<fanMissingErrDelayField>(def)),
    _setFuncOnPresent(std::get<funcOnPresentField>(def))
{
    // Setup tach sensors for monitoring
    auto& sensors = std::get<sensorListField>(def);
    for (auto& s : sensors)
    {
        _sensors.emplace_back(std::make_shared<TachSensor>(
            mode, bus, *this, std::get<sensorNameField>(s),
            std::get<hasTargetField>(s), std::get<funcDelay>(def),
            std::get<targetInterfaceField>(s), std::get<targetPathField>(s),
            std::get<factorField>(s), std::get<offsetField>(s),
            std::get<methodField>(def), std::get<thresholdField>(s),
            std::get<ignoreAboveMaxField>(s), std::get<timeoutField>(def),
            std::get<nonfuncRotorErrDelayField>(def),
            std::get<countIntervalField>(def), event));

        _trustManager->registerSensor(_sensors.back());
    }

    bool functionalState =
        (_numSensorFailsForNonFunc == 0) ||
        (countNonFunctionalSensors() < _numSensorFailsForNonFunc);

    if (updateInventory(functionalState) && !functionalState)
    {
        // the inventory update threw an exception, possibly because D-Bus
        // wasn't ready. Try to update sensors back to functional to avoid a
        // false-alarm. They will be updated again from subscribing to the
        // properties-changed event

        for (auto& sensor : _sensors)
            sensor->setFunctional(true);
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
    if (_system.isPowerOn())
    {
        _monitorTimer.restartOnce(std::chrono::seconds(_monitorDelay));
    }
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

void Fan::presenceIfaceAdded(sdbusplus::message_t& msg)
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

    std::for_each(_sensors.begin(), _sensors.end(), [this](auto& sensor) {
        if (_present)
        {
            try
            {
                // Force a getProperty call to check if the tach sensor is
                // on D-Bus.  If it isn't, now set it to nonfunctional.
                // This isn't done earlier so that code watching for
                // nonfunctional tach sensors doesn't take actions before
                // those sensors show up on D-Bus.
                sensor->updateTachAndTarget();
                tachChanged(*sensor);
            }
            catch (const util::DBusServiceError& e)
            {
                // The tach property still isn't on D-Bus. Ensure
                // sensor is nonfunctional, but skip creating an
                // error for it since it isn't a fan problem.
                getLogger().log(fmt::format(
                    "Monitoring starting but {} sensor value not on D-Bus",
                    sensor->name()));

                sensor->setFunctional(false, true);

                if (_numSensorFailsForNonFunc)
                {
                    if (_functional && (countNonFunctionalSensors() >=
                                        _numSensorFailsForNonFunc))
                    {
                        updateInventory(false);
                    }
                }

                // At this point, don't start any power off actions due
                // to missing sensors.  Let something else handle that
                // policy.
                _system.fanStatusChange(*this, true);
            }
        }
    });
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

    // If the error checking method is 'count', if a tach change leads
    // to an out of range sensor the count timer will take over in calling
    // process() until the sensor is healthy again.
    if (!sensor.countTimerRunning())
    {
        process(sensor);
    }
}

void Fan::countTimerExpired(TachSensor& sensor)
{
    if (_trustManager->active() && !_trustManager->checkTrust(sensor))
    {
        return;
    }
    process(sensor);
}

void Fan::process(TachSensor& sensor)
{
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

                    if (!sensor.countTimerRunning())
                    {
                        sensor.startCountTimer();
                    }
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
                if (sensor.getCounter() == 0)
                {
                    if (!sensor.functional())
                    {
                        updateState(sensor);
                    }

                    sensor.stopCountTimer();
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

size_t Fan::countNonFunctionalSensors() const
{
    return std::count_if(_sensors.begin(), _sensors.end(),
                         [](const auto& s) { return !s->functional(); });
}

bool Fan::outOfRange(const TachSensor& sensor)
{
    if (!sensor.hasOwner())
    {
        return true;
    }

    auto actual = static_cast<uint64_t>(sensor.getInput());
    auto range = sensor.getRange(_deviation);

    return ((actual < range.first) ||
            (range.second && actual > range.second.value()));
}

void Fan::updateState(TachSensor& sensor)
{
    if (!_system.isPowerOn())
    {
        return;
    }

    auto range = sensor.getRange(_deviation);
    std::string rangeMax = "NoMax";
    if (range.second)
    {
        rangeMax = std::to_string(range.second.value());
    }

    // Skip starting the error timer if the sensor
    // isn't on D-Bus as this isn't a fan hardware problem.
    sensor.setFunctional(!sensor.functional(), !sensor.hasOwner());

    getLogger().log(fmt::format(
        "Setting tach sensor {} functional state to {}. "
        "[target = {}, actual = {}, allowed range = ({} - {}) "
        "owned = {}]",
        sensor.name(), sensor.functional(), sensor.getTarget(),
        sensor.getInput(), range.first, rangeMax, sensor.hasOwner()));

    // A zero value for _numSensorFailsForNonFunc means we aren't dealing
    // with fan FRU functional status, only sensor functional status.
    if (_numSensorFailsForNonFunc)
    {
        auto numNonFuncSensors = countNonFunctionalSensors();
        // If the fan was nonfunctional and enough sensors are now OK,
        // the fan can be set to functional as long as `set_func_on_present` was
        // not set
        if (!_setFuncOnPresent && !_functional &&
            !(numNonFuncSensors >= _numSensorFailsForNonFunc))
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

    // Skip the power off rule checks if the sensor isn't
    // on D-Bus so a running system isn't shutdown.
    _system.fanStatusChange(*this, !sensor.hasOwner());
}

bool Fan::updateInventory(bool functional)
{
    bool dbusError = false;

    try
    {
        auto objectMap =
            util::getObjMap<bool>(_name, util::OPERATIONAL_STATUS_INTF,
                                  util::FUNCTIONAL_PROPERTY, functional);

        auto response = util::SDBusPlus::callMethod(
            _bus, util::INVENTORY_SVC, util::INVENTORY_PATH,
            util::INVENTORY_INTF, "Notify", objectMap);

        if (response.is_method_error())
        {
            log<level::ERR>("Error in Notify call to update inventory");

            dbusError = true;
        }
    }
    catch (const util::DBusError& e)
    {
        dbusError = true;

        getLogger().log(
            fmt::format("D-Bus Exception reading/updating inventory : {}",
                        e.what()),
            Logger::error);
    }

    // This will always track the current state of the inventory.
    _functional = functional;

    return dbusError;
}

void Fan::presenceChanged(sdbusplus::message_t& msg)
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

        if (_present && _setFuncOnPresent)
        {
            updateInventory(true);
            std::for_each(_sensors.begin(), _sensors.end(), [](auto& sensor) {
                sensor->setFunctional(true);
                sensor->resetMethod();
            });
        }

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

void Fan::powerStateChanged([[maybe_unused]] bool powerStateOn)
{
#ifdef MONITOR_USE_JSON
    if (powerStateOn)
    {
        _monitorTimer.restartOnce(std::chrono::seconds(_monitorDelay));

        _numSensorsOnDBusAtPowerOn = 0;

        std::for_each(_sensors.begin(), _sensors.end(), [this](auto& sensor) {
            try
            {
                // Force a getProperty call.  If sensor is on D-Bus,
                // then make sure it's functional.
                sensor->updateTachAndTarget();

                _numSensorsOnDBusAtPowerOn++;

                if (_present)
                {
                    // If not functional, set it back to functional.
                    if (!sensor->functional())
                    {
                        sensor->setFunctional(true);
                        _system.fanStatusChange(*this, true);
                    }

                    // Set the counters back to zero
                    if (sensor->getMethod() == MethodMode::count)
                    {
                        sensor->resetMethod();
                    }
                }
            }
            catch (const util::DBusError& e)
            {
                // Properties still aren't on D-Bus.  Let startMonitor()
                // deal with it, or maybe System::powerStateChanged() if
                // there aren't any sensors at all on D-Bus.
                getLogger().log(fmt::format(
                    "At power on, tach sensor {} value not on D-Bus",
                    sensor->name()));
            }
        });

        if (_present)
        {
            // If configured to change functional state on the fan itself,
            // Set it back to true now if necessary.
            if (_numSensorFailsForNonFunc)
            {
                if (!_functional &&
                    (countNonFunctionalSensors() < _numSensorFailsForNonFunc))
                {
                    updateInventory(true);
                }
            }
        }
        else
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

            sensor->stopCountTimer();
        });
    }
#endif
}

} // namespace monitor
} // namespace fan
} // namespace phosphor
