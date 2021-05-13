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
#include "manager.hpp"
#include "signal.hpp"
#include "timer.hpp"

#include <nlohmann/json.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace phosphor::fan::control::json::trigger
{

using json = nlohmann::json;

// Trigger creation function
using createTrigger =
    std::function<void(const json&, const std::string&, Manager*,
                       std::vector<std::unique_ptr<ActionBase>>&)>;

// Mapping of trigger class name to its creation function
static const std::map<std::string, createTrigger> triggers = {
    {"timer", timer::triggerTimer}, {"signal", signal::triggerSignal}};

} // namespace phosphor::fan::control::json::trigger
