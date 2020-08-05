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

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

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
     * @brief Object that parses and retrieves fan control manager attributes
     * Fan control manager attributes are optional, therefore the "manager.json"
     * file is also optional.
     *
     * @param[in] bus - sdbusplus bus object
     */
    explicit Manager(sdbusplus::bus::bus& bus);

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
};

} // namespace phosphor::fan::control::json
