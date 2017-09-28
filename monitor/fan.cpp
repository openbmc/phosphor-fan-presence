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
#include <algorithm>
#include <phosphor-logging/log.hpp>
#include "fan.hpp"
#include "types.hpp"
#include "utility.hpp"

namespace phosphor
{
namespace fan
{
namespace monitor
{

using namespace phosphor::logging;

constexpr auto INVENTORY_PATH = "/xyz/openbmc_project/inventory";
constexpr auto INVENTORY_INTF = "xyz.openbmc_project.Inventory.Manager";

constexpr auto FUNCTIONAL_PROPERTY = "Functional";
constexpr auto OPERATIONAL_STATUS_INTF  =
    "xyz.openbmc_project.State.Decorator.OperationalStatus";


Fan::Fan(Mode mode,
         sdbusplus::bus::bus& bus,
         phosphor::fan::event::EventPtr&  events,
         std::unique_ptr<trust::Manager>& trust,
         const FanDefinition& def) :
    _bus(bus),
    _name(std::get<fanNameField>(def)),
    _deviation(std::get<fanDeviationField>(def)),
    _numSensorFailsForNonFunc(std::get<numSensorFailsForNonfuncField>(def)),
    _trustManager(trust)
{
    //Start from a known state of functional
    updateInventory(true);

    // Setup tach sensors for monitoring when in monitor mode
    if (mode != Mode::init)
    {
        auto& sensors = std::get<sensorListField>(def);
        for (auto& s : sensors)
        {
            try
            {
                _sensors.emplace_back(
                        std::make_unique<TachSensor>(
                                bus,
                                *this,
                                std::get<sensorNameField>(s),
                                std::get<hasTargetField>(s),
                                std::get<timeoutField>(def),
                                events));

                _trustManager->registerSensor(_sensors.back());
            }
            catch (InvalidSensorError& e)
            {

            }
        }

        //The TachSensors will now have already read the input
        //and target values, so check them.
        tachChanged();
    }
}


void Fan::tachChanged()
{
    for (auto& s : _sensors)
    {
        tachChanged(*s);
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

    auto running = sensor.timerRunning();

    //If this sensor is out of range at this moment, start
    //its timer, at the end of which the inventory
    //for the fan may get updated to not functional.

    //If this sensor is OK, put everything back into a good state.

    if (outOfRange(sensor))
    {
        if (sensor.functional() && !running)
        {
            sensor.startTimer();
        }
    }
    else
    {
        if (!sensor.functional())
        {
            sensor.setFunctional(true);
        }

        if (running)
        {
            sensor.stopTimer();
        }

        //If the fan was nonfunctional and enough sensors are now OK,
        //the fan can go back to functional
        if (!_functional && !tooManySensorsNonfunctional())
        {
            log<level::INFO>("Setting a fan back to functional",
                             entry("FAN=%s", _name.c_str()));

            updateInventory(true);
        }
    }
}


uint64_t Fan::getTargetSpeed(const TachSensor& sensor)
{
    uint64_t target = 0;

    if (sensor.hasTarget())
    {
        target = sensor.getTarget();
    }
    else
    {
        //The sensor doesn't support a target,
        //so get it from another sensor.
        auto s = std::find_if(_sensors.begin(), _sensors.end(),
                              [](const auto& s)
                              {
                                  return s->hasTarget();
                              });

        if (s != _sensors.end())
        {
            target = (*s)->getTarget();
        }
    }

    return target;
}


bool Fan::tooManySensorsNonfunctional()
{
    size_t numFailed =  std::count_if(_sensors.begin(), _sensors.end(),
                                      [](const auto& s)
                                      {
                                          return !s->functional();
                                      });

    return (numFailed >= _numSensorFailsForNonFunc);
}


bool Fan::outOfRange(const TachSensor& sensor)
{
    auto actual = static_cast<uint64_t>(sensor.getInput());
    auto target = getTargetSpeed(sensor);

    uint64_t min = target * (100 - _deviation) / 100;
    uint64_t max = target * (100 + _deviation) / 100;

    if ((actual < min) || (actual > max))
    {
        return true;
    }

    return false;
}


void Fan::timerExpired(TachSensor& sensor)
{
    sensor.setFunctional(false);

    //If the fan is currently functional, but too many
    //contained sensors are now nonfunctional, update
    //the whole fan nonfunctional.

    if (_functional && tooManySensorsNonfunctional())
    {
        log<level::ERR>("Setting a fan to nonfunctional",
                entry("FAN=%s", _name.c_str()),
                entry("TACH_SENSOR=%s", sensor.name().c_str()),
                entry("ACTUAL_SPEED=%lld", sensor.getInput()),
                entry("TARGET_SPEED=%lld", getTargetSpeed(sensor)));

        updateInventory(false);
    }
}


void Fan::updateInventory(bool functional)
{
    ObjectMap objectMap = getObjectMap(functional);
    std::string service;

    service = phosphor::fan::util::getInvService(_bus);

    auto msg = _bus.new_method_call(service.c_str(),
                                   INVENTORY_PATH,
                                   INVENTORY_INTF,
                                   "Notify");

    msg.append(std::move(objectMap));
    auto response = _bus.call(msg);
    if (response.is_method_error())
    {
        log<level::ERR>("Error in Notify call to update inventory");
        return;
    }

    //This will always track the current state of the inventory.
    _functional = functional;
}


Fan::ObjectMap Fan::getObjectMap(bool functional)
{
    ObjectMap objectMap;
    InterfaceMap interfaceMap;
    PropertyMap propertyMap;

    propertyMap.emplace(FUNCTIONAL_PROPERTY, functional);
    interfaceMap.emplace(OPERATIONAL_STATUS_INTF, std::move(propertyMap));
    objectMap.emplace(_name, std::move(interfaceMap));

    return objectMap;
}


}
}
}
