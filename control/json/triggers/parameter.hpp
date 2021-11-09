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

#include "action.hpp"
#include "group.hpp"
#include "trigger_aliases.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <vector>

namespace phosphor::fan::control::json::trigger::parameter
{

/**
 * @brief Trigger to process an event after a parameter changes
 *
 * @param[in] jsonObj - JSON object for the trigger
 * @param[in] eventName - Name of event associated to the signal
 * @param[in] actions - Actions associated with the trigger
 */
enableTrigger
    triggerParameter(const json& jsonObj, const std::string& eventName,
                     std::vector<std::unique_ptr<ActionBase>>& actions);

} // namespace phosphor::fan::control::json::trigger::parameter
