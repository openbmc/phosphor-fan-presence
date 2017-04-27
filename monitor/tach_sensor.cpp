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
#include <phosphor-logging/log.hpp>
#include "fan.hpp"
#include "tach_sensor.hpp"
#include "../utility.hpp"

namespace phosphor
{
namespace fan
{
namespace monitor
{

constexpr auto FAN_SENSOR_PATH = "/xyz/openbmc_project/sensors/fan_tach/";


TachSensor::TachSensor(sdbusplus::bus::bus& bus,
                       Fan& fan,
                       const std::string& id,
                       bool hasTarget,
                       size_t timeout) :

    _bus(bus),
    _fan(fan),
    _name(FAN_SENSOR_PATH + id),
    _hasTarget(hasTarget),
    _timeout(timeout)
{
    //TODO
}


}
}
}
