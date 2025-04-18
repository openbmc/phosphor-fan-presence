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
#include "parameter.hpp"

#include "../manager.hpp"

#include <phosphor-logging/lg2.hpp>

namespace phosphor::fan::control::json::trigger::parameter
{

using json = nlohmann::json;

enableTrigger triggerParameter(
    const json& jsonObj, const std::string& eventName,
    std::vector<std::unique_ptr<ActionBase>>& /*actions*/)
{
    if (!jsonObj.contains("parameter"))
    {
        lg2::error(
            "Event '{EVENT_NAME}' parameter trigger is missing 'parameter'",
            "EVENT_NAME", eventName);
        throw std::runtime_error(
            "Event parameter trigger is missing 'parameter'");
    }

    auto name = jsonObj["parameter"].get<std::string>();

    return [name](const std::string& /*eventName*/, Manager* /*mgr*/,
                  const std::vector<Group>& /*groups*/,
                  std::vector<std::unique_ptr<ActionBase>>& actions) {
        Manager::addParameterTrigger(name, actions);
    };
}

} // namespace phosphor::fan::control::json::trigger::parameter
