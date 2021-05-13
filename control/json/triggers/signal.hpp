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

#include <nlohmann/json.hpp>
#include <sdbusplus/message.hpp>

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace phosphor::fan::control::json::trigger::signal
{

using json = nlohmann::json;

/**
 * @brief Subscribe to a signal
 *
 * @param[in] match - Signal match string to subscribe to
 * @param[in] pkg - Data package to attach to signal
 * @param[in] mgr - Pointer to manager of the trigger
 */
void subscribe(const std::string& match, SignalPkg&& pkg,
               std::function<bool(SignalPkg&)> isSameSig, Manager* mgr);

/**
 * @brief Subscribes to a propertiesChanged signal
 *
 * @param[in] mgr - Pointer to manager of the trigger
 * @param[in] eventName - Name of event associated to the signal
 * @param[in] action - Action to be run when signal is received
 */
void propertiesChanged(Manager* mgr, const std::string& eventName,
                       std::unique_ptr<ActionBase>& action);

// Match setup function for signals
using SignalMatch = std::function<void(Manager*, const std::string&,
                                       std::unique_ptr<ActionBase>& action)>;

/* Supported signals to their corresponding match setup functions */
static const std::unordered_map<std::string, SignalMatch> signals = {
    {"properties_changed", propertiesChanged}};

/**
 * @brief Trigger to process an event after a signal is received
 *
 * @param[in] jsonObj - JSON object for the trigger
 * @param[in] eventName - Name of event associated to the signal
 * @param[in] mgr - Pointer to manager of the trigger
 * @param[in] actions - Actions associated with the trigger
 *
 * When fan control starts (or restarts), all events with 'signal' triggers are
 * subscribed to run its corresponding actions when a signal, per its
 * configuration, is received.
 */
void triggerSignal(const json& jsonObj, const std::string& eventName,
                   Manager* mgr,
                   std::vector<std::unique_ptr<ActionBase>>& actions);

} // namespace phosphor::fan::control::json::trigger::signal
