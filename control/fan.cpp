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
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <string>
#include "fan.hpp"
#include "utility.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{

// For throwing exception
using namespace phosphor::logging;
using InternalFailure = sdbusplus::xyz::openbmc_project::Common::
                            Error::InternalFailure;

constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";
constexpr auto FAN_SENSOR_PATH = "/xyz/openbmc_project/sensors/fan_tach/";
constexpr auto FAN_SENSOR_CONTROL_INTF = "xyz.openbmc_project.Control.FanSpeed";
constexpr auto FAN_TARGET_PROPERTY = "Target";


Fan::Fan(sdbusplus::bus::bus& bus, const FanDefinition& def):
    _bus(bus),
    _name(std::get<fanNamePos>(def))
{
    auto sensors = std::get<sensorListPos>(def);
    for (auto& s : sensors)
    {
        _sensors.emplace_back(FAN_SENSOR_PATH + s);
    }
}


//TODO openbmc/openbmc#1524  Can cache this value when
//openbmc/openbmc#1496 is resolved.
std::string Fan::getService(const std::string& sensor)
{
    return phosphor::fan::util::getService(sensor,
                                           FAN_SENSOR_CONTROL_INTF,
                                           _bus);
}


void Fan::setSpeed(uint64_t speed)
{
    sdbusplus::message::variant<uint64_t> value = speed;
    std::string property{FAN_TARGET_PROPERTY};

    for (auto& sensor : _sensors)
    {
        auto service = getService(sensor);

        auto method = _bus.new_method_call(service.c_str(),
                                               sensor.c_str(),
                                               PROPERTY_INTERFACE,
                                               "Set");
        method.append(FAN_SENSOR_CONTROL_INTF, property, value);

        auto response = _bus.call(method);
        if (response.is_method_error())
        {
            log<level::ERR>(
                "Failed call to set fan speed ", entry("SENSOR=%s", sensor));
            elog<InternalFailure>();
        }
    }
}

}
}
}
