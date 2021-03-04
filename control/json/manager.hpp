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

#include "json_parser.hpp"
#include "profile.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

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

    /* List of profiles configured */
    const std::map<configKey, std::unique_ptr<Profile>> _profiles;

    /* List of active profiles */
    std::vector<std::string> _activeProfiles;
};

} // namespace phosphor::fan::control::json
