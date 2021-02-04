/**
 * Copyright © 2020 IBM Corporation
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

#include "sdbusplus.hpp"

#include <fmt/format.h>

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using namespace phosphor::logging;

constexpr auto FAN_SENSOR_PATH = "/xyz/openbmc_project/sensors/fan_tach/";
constexpr auto FAN_TARGET_PROPERTY = "Target";

Fan::Fan(sdbusplus::bus::bus& bus, const json& jsonObj) :
    ConfigBase(jsonObj), _bus(bus)
{
    setInterface(jsonObj);
    setSensors(jsonObj);
    setZone(jsonObj);
}

void Fan::setInterface(const json& jsonObj)
{
    if (!jsonObj.contains("target_interface"))
    {
        log<level::ERR>("Missing required fan sensor target interface",
                        entry("JSON=%s", jsonObj.dump().c_str()));
        throw std::runtime_error(
            "Missing required fan sensor target interface");
    }
    _interface = jsonObj["target_interface"].get<std::string>();
}

void Fan::setSensors(const json& jsonObj)
{
    if (!jsonObj.contains("sensors"))
    {
        log<level::ERR>("Missing required fan sensors list",
                        entry("JSON=%s", jsonObj.dump().c_str()));
        throw std::runtime_error("Missing required fan sensors list");
    }
    std::string path;
    for (const auto& sensor : jsonObj["sensors"])
    {
        path = FAN_SENSOR_PATH + sensor.get<std::string>();
        auto service = util::SDBusPlus::getService(_bus, path, _interface);
        _sensors[path] = service;
    }
    // All sensors associated with this fan are set to the same target,
    // so only need to read target property from one of them
    if (!path.empty())
    {
        _target = util::SDBusPlus::getProperty<uint64_t>(
            _bus, _sensors.at(path), path, _interface, FAN_TARGET_PROPERTY);
    }
}

void Fan::setZone(const json& jsonObj)
{
    if (!jsonObj.contains("zone"))
    {
        log<level::ERR>("Missing required fan zone",
                        entry("JSON=%s", jsonObj.dump().c_str()));
        throw std::runtime_error("Missing required fan zone");
    }
    _zone = jsonObj["zone"].get<std::string>();
}

void Fan::setTarget(uint64_t target)
{
    for (const auto& sensor : _sensors)
    {
        auto value = target;
        try
        {
            util::SDBusPlus::setProperty<uint64_t>(
                _bus, sensor.second, sensor.first, _interface,
                FAN_TARGET_PROPERTY, std::move(value));
        }
        catch (const sdbusplus::exception::SdBusError&)
        {
            throw util::DBusPropertyError{
                fmt::format("Failed to set target for fan {}", _name).c_str(),
                sensor.second, sensor.first, _interface, FAN_TARGET_PROPERTY};
        }
    }
    _target = target;
}

} // namespace phosphor::fan::control::json
