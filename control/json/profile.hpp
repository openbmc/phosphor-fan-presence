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

#include "config_base.hpp"

#include <nlohmann/json.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using methodHandler = std::function<bool(const json&)>;

/**
 * @class Profile - Represents a configured fan control profile
 *
 * Fan control profiles are optional, therefore the "profiles.json" file is
 * also optional. A profile can be used to load specific fan control events
 * based on the configuration of the profile. Fan control events configured
 * with no profile(s) are always used and events configured for a specified
 * profile are included when that profile is enabled.
 *
 * When no profiles exist, all configured fan control events are used.
 */
class Profile : public ConfigBase
{
  public:
    /* JSON file name for profiles */
    static constexpr auto confFileName = "profiles.json";

    Profile() = delete;
    Profile(const Profile&) = delete;
    Profile(Profile&&) = delete;
    Profile& operator=(const Profile&) = delete;
    Profile& operator=(Profile&&) = delete;
    ~Profile() = default;

    /**
     * Constructor
     * Parses and populates a zone profile from JSON object data
     *
     * @param[in] bus - sdbusplus bus object
     * @param[in] jsonObj - JSON object
     */
    explicit Profile(const json& jsonObj);

    /**
     * @brief Get the active state
     *
     * @return The active state of the profile
     */
    inline bool isActive() const
    {
        return _active;
    }

  private:
    /* Active state of the profile */
    bool _active;

    /* Supported methods to their corresponding handler functions */
    static const std::map<std::string, methodHandler> _methods;

    /**
     * @brief Parse and set the profile's active state
     *
     * @param[in] jsonObj - JSON object for the profile
     *
     * Sets the active state of the profile using the configured method of
     * determining its active state.
     */
    void setActive(const json& jsonObj);

    /**
     * @brief An active state method where all must be true
     *
     * @param[in] method - JSON for the profile's method
     *
     * Active state method that takes a list of configured dbus properties where
     * all of those properties must equal their configured values to set the
     * profile to be active.
     *
     * "name": "all_of",
     * "properties": [
     *     {
     *         "path": "[DBUS PATH]",
     *         "interface": "[DBUS INTERFACE]",
     *         "property": "[DBUS PROPERTY]",
     *         "value": [VALUE TO BE ACTIVE]
     *     }
     * ]
     */
    static bool allOf(const json& method);
};

} // namespace phosphor::fan::control::json
