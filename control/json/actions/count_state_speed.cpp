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
#include "count_state_speed.hpp"

#include "action.hpp"
#include "types.hpp"
#include "zone.hpp"

#include <fmt/format.h>

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

#include <algorithm>
#include <tuple>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using namespace phosphor::logging;

CountStateSpeed::CountStateSpeed(const json& jsonObj) : ActionBase(jsonObj)
{
    setCount(jsonObj);
    setState(jsonObj);
    setSpeed(jsonObj);
}

void CountStateSpeed::run(Zone& zone, const Group& group)
{
    size_t numAtState = 0;
    for (auto& entry : group)
    {
        try
        {
            if (zone.getPropValueVariant(std::get<pathPos>(entry),
                                         std::get<intfPos>(entry),
                                         std::get<propPos>(entry)) == _state)
            {
                numAtState++;
            }
        }
        catch (const std::out_of_range& oore)
        {
            // Default to property not equal when not found
        }
        if (numAtState >= _count)
        {
            zone.setSpeed(_speed);
            break;
        }
    }
    // Update group's fan control active allowed based on action results
    zone.setActiveAllow(&group, !(numAtState >= _count));
}

void CountStateSpeed::setCount(const json& jsonObj)
{
    if (!jsonObj.contains("count"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required count value"};
    }
    _count = jsonObj["count"].get<size_t>();
}

void CountStateSpeed::setState(const json& jsonObj)
{
    if (!jsonObj.contains("state"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required state value"};
    }
    _state = getJsonValue(jsonObj["state"]);
}

void CountStateSpeed::setSpeed(const json& jsonObj)
{
    if (!jsonObj.contains("speed"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required speed value"};
    }
    _speed = jsonObj["speed"].get<uint64_t>();
}

} // namespace phosphor::fan::control::json
