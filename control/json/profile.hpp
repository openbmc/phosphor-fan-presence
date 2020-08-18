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

#include "config_attr.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

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
class Profile : public ConfigAttr
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
    Profile(sdbusplus::bus::bus& bus, const json& jsonObj);

  private:
    /* The sdbusplus bus object */
    sdbusplus::bus::bus& _bus;
};

} // namespace phosphor::fan::control::json
