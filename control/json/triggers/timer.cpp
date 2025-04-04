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
#include "timer.hpp"

#include "../manager.hpp"
#include "group.hpp"
#include "trigger_aliases.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/lg2.hpp>

#include <chrono>

namespace phosphor::fan::control::json::trigger::timer
{

using json = nlohmann::json;

TimerType getType(const json& jsonObj)
{
    if (!jsonObj.contains("type"))
    {
        lg2::error("Missing required timer trigger type", "JSON",
                   jsonObj.dump());
        throw std::runtime_error("Missing required timer trigger type");
    }
    auto type = jsonObj["type"].get<std::string>();
    if (type == "oneshot")
    {
        return TimerType::oneshot;
    }
    else if (type == "repeating")
    {
        return TimerType::repeating;
    }
    else
    {
        lg2::error(
            "Timer trigger type '{TYPE}' is not supported. Available types are 'oneshot, repeating'",
            "TYPE", type);
        throw std::runtime_error("Unsupported timer trigger type given");
    }
}

std::chrono::microseconds getInterval(const json& jsonObj)
{
    if (!jsonObj.contains("interval"))
    {
        lg2::error("Missing required timer trigger interval", "JSON",
                   jsonObj.dump());
        throw std::runtime_error("Missing required timer trigger interval");
    }
    return static_cast<std::chrono::microseconds>(
        jsonObj["interval"].get<uint64_t>());
}

bool getPreload(const json& jsonObj)
{
    if (jsonObj.contains("preload_groups") &&
        jsonObj["preload_groups"].get<bool>())
    {
        return true;
    }
    return false;
}

enableTrigger triggerTimer(
    const json& jsonObj, const std::string& /*eventName*/,
    std::vector<std::unique_ptr<ActionBase>>& /*actions*/)
{
    // Get the type and interval of this timer from the JSON
    auto type = getType(jsonObj);
    auto interval = getInterval(jsonObj);
    auto preload = getPreload(jsonObj);

    return [type = std::move(type), interval = std::move(interval),
            preload = std::move(preload)](
               const std::string& eventName, Manager* mgr,
               const std::vector<Group>& groups,
               std::vector<std::unique_ptr<ActionBase>>& actions) {
        auto tpPtr = std::make_unique<TimerPkg>(eventName, std::ref(actions),
                                                std::cref(groups), preload);
        mgr->addTimer(type, interval, std::move(tpPtr));
    };
}

} // namespace phosphor::fan::control::json::trigger::timer
