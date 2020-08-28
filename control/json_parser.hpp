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

#include "json/profile.hpp"
#include "json_config.hpp"
#include "types.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>

#include <map>
#include <memory>

namespace phosphor::fan::control
{

/* Application name to be appended to the path for loading a JSON config file */
constexpr auto confAppName = "control";

/**
 * @brief Load the configuration of a given JSON class object type
 *
 * @param[in] bus - The dbus bus object
 * @param[in] isOptional - JSON configuration file is optional or not
 *                         Defaults to false
 *
 * @return Map of configuration entries
 *     Map of configuration names to their corresponding configuration object
 */
template <typename T>
std::map<std::string, std::unique_ptr<T>> getConfig(sdbusplus::bus::bus& bus,
                                                    bool isOptional = false)
{
    std::map<std::string, std::unique_ptr<T>> config;

    auto confFile = fan::JsonConfig::getConfFile(bus, confAppName,
                                                 T::confFileName, isOptional);
    if (!confFile.empty())
    {
        for (const auto& entry : fan::JsonConfig::load(confFile))
        {
            auto obj = std::make_unique<T>(bus, entry);
            config.emplace(obj->getName(), std::move(obj));
        }
    }

    return config;
}

/**
 * @brief Get the configuration definitions for zone groups
 *
 * @param[in] bus - The dbus bus object
 *
 * @return List of zone group objects
 *     Generated list of zone groups including their control configurations
 */
const std::vector<ZoneGroup> getZoneGroups(sdbusplus::bus::bus& bus);

/**
 * @brief Get the delay(in seconds) to allow the fans to ramp up to the defined
 * power on speed
 *
 * @param[in] bus - The dbus bus object
 *
 * @return Time to delay in seconds
 *     Amount of time to delay in seconds after a power on
 */
const unsigned int getPowerOnDelay(sdbusplus::bus::bus& bus);

} // namespace phosphor::fan::control
