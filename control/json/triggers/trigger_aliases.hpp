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
#include "manager.hpp"

#include <nlohmann/json.hpp>

#include <functional>
#include <memory>
#include <vector>

namespace phosphor::fan::control::json::trigger
{

using json = nlohmann::json;

// Trigger enablement function
using enableTrigger =
    std::function<void(const std::string&, Manager*, const std::vector<Group>&,
                       std::vector<std::unique_ptr<ActionBase>>&)>;

// Trigger creation function
using createTrigger =
    std::function<enableTrigger(const json&, const std::string&,
                                std::vector<std::unique_ptr<ActionBase>>&)>;

} // namespace phosphor::fan::control::json::trigger
