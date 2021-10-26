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
#include "count_state_floor.hpp"

#include "../manager.hpp"
#include "../zone.hpp"
#include "action.hpp"
#include "group.hpp"

namespace phosphor::fan::control::json
{

CountStateFloor::CountStateFloor(const json& jsonObj,
                                 const std::vector<Group>& groups) :
    ActionBase(jsonObj, groups)
{
    setCount(jsonObj);
    setState(jsonObj);
    setFloor(jsonObj);
}

void CountStateFloor::run(Zone& zone)
{
    size_t numAtState = 0;
    for (const auto& group : _groups)
    {
        for (const auto& member : group.getMembers())
        {
            try
            {
                if (Manager::getObjValueVariant(member, group.getInterface(),
                                                group.getProperty()) == _state)
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
                break;
            }
        }
        if (numAtState >= _count)
        {
            break;
        }
    }

    // Update zone's floor hold based on action results
    zone.setFloorHold(getUniqueName(), _floor, (numAtState >= _count));
}

void CountStateFloor::setCount(const json& jsonObj)
{
    if (!jsonObj.contains("count"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required count value"};
    }
    _count = jsonObj["count"].get<size_t>();
}

void CountStateFloor::setState(const json& jsonObj)
{
    if (!jsonObj.contains("state"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required state value"};
    }
    _state = getJsonValue(jsonObj["state"]);
}

void CountStateFloor::setFloor(const json& jsonObj)
{
    if (!jsonObj.contains("floor"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required floor value"};
    }
    _floor = jsonObj["floor"].get<uint64_t>();
}

} // namespace phosphor::fan::control::json
