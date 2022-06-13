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
#include "power_off_action.hpp"
#include "trust_group.hpp"
#include "types.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>

namespace phosphor::fan::monitor
{

using json = nlohmann::json;
class PowerOffRule;
class PowerInterfaceBase;
class System;

constexpr auto confAppName = "monitor";
constexpr auto confFileName = "config.json";

// Trust group class handler function
using trustHandler = std::function<CreateGroupFunction(
    const std::vector<trust::GroupDefinition>&)>;
// Fan monitoring condition handler function
using condHandler = std::function<Condition(const json&)>;

/**
 * @brief Get the JSON object
 *
 * @return JSON object
 *     A JSON object created after loading the JSON configuration file
 */
inline const json getJsonObj()
{
    return fan::JsonConfig::load(
        fan::JsonConfig::getConfFile(confAppName, confFileName));
}

/**
 * @brief Get any configured trust groups
 *
 * @param[in] obj - JSON object to parse from
 *
 * @return List of functions applied on trust groups
 */
const std::vector<CreateGroupFunction> getTrustGrps(const json& obj);

/**
 * @brief Get the configured sensor definitions that make up a fan
 *
 * @param[in] sensor - JSON object containing a list of sensors
 *
 * @return List of sensor definition data that make up a fan being monitored
 */
const std::vector<SensorDefinition> getSensorDefs(const json& sensors);

/**
 * @brief Get the configured fan definitions to be monitored
 *
 * @param[in] obj - JSON object to parse from
 *
 * @return List of fan definition data on the fans to be monitored
 */
const std::vector<FanDefinition> getFanDefs(const json& obj);

/**
 * @brief Get the configured power off rules
 *
 * @param[in] obj - JSON object to parse from
 *
 * @param[in] powerInterface - The power interface object to use
 *
 * @param[in] func - Optional user defined function that gets called
 *                   right before a power off occurs.
 *
 * @return std::vector<std::unique_ptr<PowerOffRule>> -
 *     The PowerOffRule objects
 */
std::vector<std::unique_ptr<PowerOffRule>>
    getPowerOffRules(const json& obj,
                     std::shared_ptr<PowerInterfaceBase>& powerInterface,
                     PowerOffAction::PrePowerOffFunc& func);

/**
 * @brief Returns the 'num_nonfunc_rotors_before_error field
 *
 * @param[in] obj - JSON object to parse from
 *
 * @return optional<size_t> - The value, or std::nullopt if not present
 */
std::optional<size_t> getNumNonfuncRotorsBeforeError(const json& obj);

} // namespace phosphor::fan::monitor
