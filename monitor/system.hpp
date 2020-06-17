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

#include "fan.hpp"
#include "tach_sensor.hpp"
#include "trust_manager.hpp"
#include "types.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

#include <memory>
#include <optional>
#include <vector>

namespace phosphor::fan::monitor
{

using json = nlohmann::json;

class System
{
  public:
    System() = delete;
    System(const System&) = delete;
    System(System&&) = delete;
    System& operator=(const System&) = delete;
    System& operator=(System&&) = delete;
    ~System() = default;

    /**
     * Constructor
     * Parses and populates the fan monitor trust groups and list of fans
     *
     * @param[in] mode - mode of fan monitor
     * @param[in] bus - sdbusplus bus object
     * @param[in] event - event loop reference
     */
    System(Mode mode, sdbusplus::bus::bus& bus,
           const sdeventplus::Event& event);

  private:
    /* The mode of fan monitor */
    Mode _mode;

    /* The sdbusplus bus object */
    sdbusplus::bus::bus& _bus;

    /* The event loop reference */
    const sdeventplus::Event& _event;

    /* Trust manager of trust groups */
    std::unique_ptr<phosphor::fan::trust::Manager> _trust;

    /* List of fan objects to monitor */
    std::vector<std::unique_ptr<Fan>> _fans;

    /**
     * @brief Retrieve the configured trust groups
     *
     * @param[in] jsonObj - JSON object to parse from
     *
     * @return List of functions applied on trust groups
     */
    const std::vector<CreateGroupFunction> getTrustGroups(const json& jsonObj);

    /**
     * @brief Retrieve the configured fan definitions
     *
     * @param[in] jsonObj - JSON object to parse from
     *
     * @return List of fan definition data on the fans configured
     */
    const std::vector<FanDefinition> getFanDefinitions(const json& jsonObj);
};

} // namespace phosphor::fan::monitor
