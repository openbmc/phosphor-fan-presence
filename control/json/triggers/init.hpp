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
#include "group.hpp"
#include "trigger_aliases.hpp"

#include <nlohmann/json.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace phosphor::fan::control::json::trigger::init
{

using json = nlohmann::json;

/**
 * @brief An init method to get properties used in an event
 *
 * @param[in] mgr - Pointer to manager of the event
 * @param[in] group - Group associated with the event
 */
void getProperties(Manager* mgr, const Group& group);

/**
 * @brief An init method to get the owner name of a service used in an event
 *
 * @param[in] mgr - Pointer to manager of the event
 * @param[in] group - Group associated with the event
 */
void nameHasOwner(Manager* mgr, const Group& group);

// Handler function for method calls
using methodHandler = std::function<void(Manager*, const Group&)>;

/* Supported methods to their corresponding handler functions */
static const std::map<std::string, methodHandler> methods = {
    {"get_properties", getProperties}, {"name_has_owner", nameHasOwner}};

/**
 * @brief Trigger to process an event immediately upon fan control starting
 *
 * @param[in] jsonObj - JSON object for the trigger
 * @param[in] eventName - Name of event creating the trigger
 * @param[in] actions - Actions associated with the trigger
 *
 * When fan control starts (or restarts), all events with 'init' triggers are
 * processed immediately, per its configuration, and its corresponding actions
 * are run.
 *
 * Generally, this type of trigger is paired with a 'signal' class of trigger on
 * an event so the initial data for an event is collected, processed, and run
 * before any signal may be received.
 */
enableTrigger triggerInit(const json& jsonObj, const std::string& eventName,
                          std::vector<std::unique_ptr<ActionBase>>& actions);

} // namespace phosphor::fan::control::json::trigger::init
