/**
 * Copyright © 2021 IBM Corporation
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

#include "../manager.hpp"
#include "action.hpp"
#include "trigger_aliases.hpp"

#include <nlohmann/json.hpp>

#include <chrono>

namespace phosphor::fan::control::json::trigger::timer
{

using json = nlohmann::json;

/**
 * @brief Parse and return the timer trigger's type
 *
 * @param[in] jsonObj - JSON object for the timer trigger
 *
 * Gets the type of this timer
 */
TimerType getType(const json& jsonObj);

/**
 * @brief Parse and return the timer's interval
 *
 * @param[in] jsonObj - JSON object for the timer trigger
 *
 * Gets the interval of this timer
 */
std::chrono::microseconds getInterval(const json& jsonObj);

/**
 * @brief Trigger to run an event based on a timer
 *
 * @param[in] jsonObj - JSON object for the trigger
 * @param[in] eventName - Name of event creating the trigger
 * @param[in] actions - Actions associated with the trigger
 *
 * When fan control starts (or restarts), all events with 'timer' triggers
 * have their timers started. Once the timer expires, per its configuration,
 * the corresponding event's actions are run.
 */
enableTrigger triggerTimer(const json& jsonObj, const std::string& eventName,
                           std::vector<std::unique_ptr<ActionBase>>& actions);

} // namespace phosphor::fan::control::json::trigger::timer
