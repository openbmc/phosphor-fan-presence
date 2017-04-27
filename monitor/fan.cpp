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

namespace phosphor
{
namespace fan
{
namespace monitor
{

using namespace phosphor::logging;

Fan::Fan(sdbusplus::bus::bus& bus,
         std::shared_ptr<sd_event>&  events,
         const FanDefinition& def) :
    _bus(bus),
    _name(std::get<fanNameField>(def)),
    _deviation(std::get<fanDeviationField>(def)),
    _numSensorFailsForNonFunc(std::get<numSensorFailsForNonfuncField>(def))
{
    auto& sensors = std::get<sensorListField>(def);

    for (auto& s : sensors)
    {
        _sensors.emplace_back(
            std::make_unique<TachSensor>(bus,
                                         *this,
                                         std::get<sensorNameField>(s),
                                         std::get<hasTargetField>(s),
                                         std::get<timeoutField>(def),
                                         events));
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
    //TODO
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


void Fan::timerExpired(TachSensor* sensor)
{
    sensor->setFunctional(false);

    //If the fan is currently functional, but too many
    //contained sensors are now nonfunctional, update
    //the whole fan nonfunctional.
	//TODO
}

}
}
}
