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

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using namespace phosphor::logging;

constexpr auto FAN_SENSOR_PATH = "/xyz/openbmc_project/sensors/fan_tach/";

Fan::Fan(sdbusplus::bus::bus& bus, const json& jsonObj) : ConfigBase(jsonObj)
{
    if (jsonObj.contains("profiles"))
    {
        for (const auto& profile : jsonObj["profiles"])
        {
            _profiles.emplace_back(profile.get<std::string>());
        }
    }
    setZone(jsonObj);
    setSensors(jsonObj);
    setInterface(jsonObj);
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

void Fan::setSensors(const json& jsonObj)
{
    if (!jsonObj.contains("sensors"))
    {
        log<level::ERR>("Missing required fan sensors list",
                        entry("JSON=%s", jsonObj.dump().c_str()));
        throw std::runtime_error("Missing required fan sensors list");
    }
    for (const auto& sensor : jsonObj["sensors"])
    {
        _sensors.emplace_back(FAN_SENSOR_PATH + sensor.get<std::string>());
    }
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

} // namespace phosphor::fan::control::json
