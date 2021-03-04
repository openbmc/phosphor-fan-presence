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
#include "types.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>

#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace phosphor::fan::control
{

/* Application name to be appended to the path for loading a JSON config file */
constexpr auto confAppName = "control";

/**
 * Configuration object key to uniquely map to the configuration object
 * Pair constructed of:
 *      std::string = Configuration object's name
 *      std::vector<std::string> = List of profiles the configuration object
 *                                 is included in
 */
using configKey = std::pair<std::string, std::vector<std::string>>;

/**
 * @brief Load the configuration of a given JSON class object type that requires
 * a dbus object
 *
 * @param[in] bus - The dbus bus object
 * @param[in] isOptional - JSON configuration file is optional or not
 *                         Defaults to false
 *
 * @return Map of configuration entries
 *     Map of configuration keys to their corresponding configuration object
 */
template <typename T>
std::map<configKey, std::unique_ptr<T>> getConfig(sdbusplus::bus::bus& bus,
                                                  bool isOptional = false)
{
    std::map<configKey, std::unique_ptr<T>> config;

    auto confFile = fan::JsonConfig::getConfFile(bus, confAppName,
                                                 T::confFileName, isOptional);
    if (!confFile.empty())
    {
        for (const auto& entry : fan::JsonConfig::load(confFile))
        {
            auto obj = std::make_unique<T>(bus, entry);
            config.emplace(std::make_pair(obj->getName(), obj->getProfiles()),
                           std::move(obj));
        }
    }
    return config;
}

/**
 * @brief Load the configuration of a given JSON class object type that does not
 * require a dbus object
 *
 * @param[in] isOptional - JSON configuration file is optional or not
 *                         Defaults to false
 *
 * @return Map of configuration entries
 *     Map of configuration keys to their corresponding configuration object
 */
template <typename T>
std::map<configKey, std::unique_ptr<T>> getConfig(bool isOptional = false)
{
    // Dbus object is needed to retrieve the JSON configuration file
    auto bus = sdbusplus::bus::new_default();
    std::map<configKey, std::unique_ptr<T>> config;

    auto confFile = fan::JsonConfig::getConfFile(bus, confAppName,
                                                 T::confFileName, isOptional);
    if (!confFile.empty())
    {
        for (const auto& entry : fan::JsonConfig::load(confFile))
        {
            auto obj = std::make_unique<T>(entry);
            config.emplace(std::make_pair(obj->getName(), obj->getProfiles()),
                           std::move(obj));
        }
    }
    return config;
}

/**
 * @brief Helper function to determine when a configuration entry is included
 * based on the list of active profiles and its list of profiles
 *
 * A configuration entry may include a list of profiles that when any one of
 * the profiles are active, the entry should be included. When the list of
 * profiles for a configuration entry is empty, that results in always
 * including the entry. An empty list of active profiles results in including
 * only those entries configured without a list of profiles.
 *
 * i.e.) No profiles configured results in always being included, whereas
 * providing a list of profiles on an entry results only in that entry being
 * included when a profile in the list is active.
 *
 * @param[in] activeProfiles - List of active system profiles
 * @param[in] entryProfiles - List of the configuration entry's profiles
 *
 * @return Whether the configuration entry should be included or not
 */
bool checkEntry(const std::vector<std::string>& activeProfiles,
                const std::vector<std::string>& entryProfiles);

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
const unsigned int getPowerOnDelay(sdbusplus::bus::bus& bus,
                                   const sdeventplus::Event& event);

} // namespace phosphor::fan::control
