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
#pragma once

#include "json_config.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>

namespace phosphor::fan::monitor
{

using json = nlohmann::json;

constexpr auto confAppName = "monitor";
constexpr auto confFileName = "config.json";

/**
 * @brief Get the JSON object
 *
 * @param[in] bus - The dbus bus object
 *
 * @return JSON object
 *     A JSON object created after loading the JSON configuration file
 */
inline const json getJsonObj(sdbusplus::bus::bus& bus)
{
    return fan::JsonConfig::load(
        fan::JsonConfig::getConfFile(bus, confAppName, confFileName));
}

} // namespace phosphor::fan::monitor
