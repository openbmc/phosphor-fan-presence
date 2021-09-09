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
#include "failure_target_override.hpp"

#include "../manager.hpp"
#include "../zone.hpp"
#include "action.hpp"
#include "group.hpp"

#include <fmt/format.h>

#include <nlohmann/json.hpp>

#include <iostream>
using std::cout;
using std::endl;

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

// Instance id for setting the unique id of each instance of this action
size_t FailureOverrideTarget::instanceId = 0;

FailureOverrideTarget::FailureOverrideTarget(const json& jsonObj,
                                             const std::vector<Group>& groups) :
    ActionBase(jsonObj, groups)
{
    setCount(jsonObj);
    setState(jsonObj);
    setTarget(jsonObj);

    _id = instanceId++;
}

void FailureOverrideTarget::run(Zone& zone)
{
    size_t numAtState = 0;
    cout << "calling overide::run() " << endl;
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
                cout << "Setting override target: " << member << endl;
                zone.setTarget(member, _target);
                break;
            }
        }
        if (numAtState >= _count)
        {
            break;
        }
    }

    // Update zone's active fan control allowed based on action results
    zone.setActiveAllow(ActionBase::getName() + std::to_string(_id),
                        !(numAtState >= _count));
}

void FailureOverrideTarget::setCount(const json& jsonObj)
{
    if (!jsonObj.contains("count"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required count value"};
    }
    _count = jsonObj["count"].get<size_t>();
}

void FailureOverrideTarget::setState(const json& jsonObj)
{
    if (!jsonObj.contains("state"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required state value"};
    }
    _state = getJsonValue(jsonObj["state"]);
}

void FailureOverrideTarget::setTarget(const json& jsonObj)
{
    if (!jsonObj.contains("target"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required target value"};
    }
    _target = jsonObj["target"].get<uint64_t>();
}

} // namespace phosphor::fan::control::json
