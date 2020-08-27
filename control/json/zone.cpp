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
#include "zone.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using namespace phosphor::logging;

Zone::Zone(sdbusplus::bus::bus& bus, const json& jsonObj) :
    ConfigBase(jsonObj), _incDelay(0)
{
    if (jsonObj.contains("profiles"))
    {
        for (const auto& profile : jsonObj["profiles"])
        {
            _profiles.emplace_back(profile.get<std::string>());
        }
    }
    // Speed increase delay is optional, defaults to 0
    if (jsonObj.contains("increase_delay"))
    {
        _incDelay = jsonObj["increase_delay"].get<uint64_t>();
    }
    setFullSpeed(jsonObj);
    setDefaultFloor(jsonObj);
    setDecInterval(jsonObj);
}

void Zone::setFullSpeed(const json& jsonObj)
{
    if (!jsonObj.contains("full_speed"))
    {
        log<level::ERR>("Missing required zone's full speed",
                        entry("JSON=%s", jsonObj.dump().c_str()));
        throw std::runtime_error("Missing required zone's full speed");
    }
    _fullSpeed = jsonObj["full_speed"].get<uint64_t>();
}

void Zone::setDefaultFloor(const json& jsonObj)
{
    if (!jsonObj.contains("default_floor"))
    {
        log<level::ERR>("Missing required zone's default floor speed",
                        entry("JSON=%s", jsonObj.dump().c_str()));
        throw std::runtime_error("Missing required zone's default floor speed");
    }
    _defaultFloor = jsonObj["default_floor"].get<uint64_t>();
}

void Zone::setDecInterval(const json& jsonObj)
{
    if (!jsonObj.contains("decrease_interval"))
    {
        log<level::ERR>("Missing required zone's decrease interval",
                        entry("JSON=%s", jsonObj.dump().c_str()));
        throw std::runtime_error("Missing required zone's decrease interval");
    }
    _decInterval = jsonObj["decrease_interval"].get<uint64_t>();
}

} // namespace phosphor::fan::control::json
