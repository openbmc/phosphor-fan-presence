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
#include "profile.hpp"
#include "zone.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

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
 * @class Manager - Represents the fan control manager's configuration
 *
 * A fan control manager configuration is optional, therefore the "manager.json"
 * file is also optional. The manager configuration is used to populate
 * fan control's manager parameters which are used in how the application
 * operates, not in how the fans are controlled.
 *
 * When no manager configuration exists, the fan control application starts,
 * processes any configured events and then begins controlling fans according
 * to those events.
 */
class Manager
{
  public:
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager& operator=(Manager&&) = delete;
    ~Manager() = default;

    /**
     * Constructor
     * Parses and populates the fan control manager attributes from a json file
     *
     * @param[in] bus - sdbusplus bus object
     * @param[in] event - sdeventplus event loop
     */
    Manager(sdbusplus::bus::bus& bus, const sdeventplus::Event& event);

    /**
     * @brief Get the active profiles of the system where an empty list
     * represents that only configuration entries without a profile defined will
     * be loaded.
     *
     * @return - The list of active profiles
     */
    static const std::vector<std::string>& getActiveProfiles();

    /**
     * @brief Load the configuration of a given JSON class object based on the
     * active profiles
     *
     * @param[in] bus - The sdbusplus bus object
     * @param[in] isOptional - JSON configuration file is optional or not
     *                         Defaults to false
     *
     * @return Map of configuration entries
     *     Map of configuration keys to their corresponding configuration object
     */
    template <typename T>
    static std::map<configKey, std::unique_ptr<T>>
        getConfig(sdbusplus::bus::bus& bus, bool isOptional = false)
    {
        std::map<configKey, std::unique_ptr<T>> config;

        auto confFile = fan::JsonConfig::getConfFile(
            bus, confAppName, T::confFileName, isOptional);
        if (!confFile.empty())
        {
            for (const auto& entry : fan::JsonConfig::load(confFile))
            {
                if (entry.contains("profiles"))
                {
                    std::vector<std::string> profiles;
                    for (const auto& profile : entry["profiles"])
                    {
                        profiles.emplace_back(
                            profile.template get<std::string>());
                    }
                    // Do not create the object if its profiles are not in the
                    // list of active profiles
                    if (!std::any_of(profiles.begin(), profiles.end(),
                                     [](const auto& name) {
                                         return std::find(
                                                    getActiveProfiles().begin(),
                                                    getActiveProfiles().end(),
                                                    name) !=
                                                getActiveProfiles().end();
                                     }))
                    {
                        continue;
                    }
                }
                auto obj = std::make_unique<T>(bus, entry);
                config.emplace(
                    std::make_pair(obj->getName(), obj->getProfiles()),
                    std::move(obj));
            }
        }
        return config;
    }

    /**
     * @brief Get the configured power on delay(OPTIONAL)
     *
     * @return Power on delay in seconds
     *     Configured power on delay in seconds, otherwise 0
     */
    unsigned int getPowerOnDelay();

  private:
    /* JSON file name for manager configuration attributes */
    static constexpr auto confFileName = "manager.json";

    /* The parsed JSON object */
    json _jsonObj;

    /**
     * The sdbusplus bus object to use
     */
    sdbusplus::bus::bus& _bus;

    /**
     * The sdeventplus even loop to use
     */
    sdeventplus::Event _event;

    /* List of profiles configured */
    std::map<configKey, std::unique_ptr<Profile>> _profiles;

    /* List of active profiles */
    static std::vector<std::string> _activeProfiles;

    /* List of zones configured */
    std::map<configKey, std::unique_ptr<Zone>> _zones;

    /**
     * @brief Parse and set the configured profiles from the profiles JSON file
     *
     * Retrieves the optional profiles JSON configuration file, parses it, and
     * creates a list of configured profiles available to the other
     * configuration files. These profiles can be used to remove or include
     * entries within the other configuration files.
     */
    void setProfiles();
};

} // namespace phosphor::fan::control::json
