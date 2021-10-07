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
#include "override_fan_target.hpp"

#include "../manager.hpp"
#include "../zone.hpp"
#include "action.hpp"
#include "group.hpp"

#include <fmt/format.h>

#include <nlohmann/json.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

// Instance id for setting the unique id of each instance of this action
size_t OverrideFanTarget::instanceId = 0;

OverrideFanTarget::OverrideFanTarget(const json& jsonObj,
                                     const std::vector<Group>& groups) :
    ActionBase(jsonObj, groups)
{
    setCount(jsonObj);
    setState(jsonObj);
    setTarget(jsonObj);

    // not sure if this really needs its own func
    _fans = jsonObj["fans"].get<std::vector<std::string>>();
    _id = instanceId++;
}

void OverrideFanTarget::run(Zone& zone)
{
    bool locked = false;

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
            catch (const std::out_of_range&)
            {}

            // shortcut to activate Fan lock
            if (numAtState >= _count)
            {
                goto lockFans;
            }
        }
    }

    if (numAtState < _count)
    {
        if (locked)
        {
            record("Zone fans unlocked.");

            for (auto& fan : _fans)
            {
                zone.unlockFanTarget(fan);
            }

            locked = false;
        }
    }
    else
    {
    lockFans:
        std::string recordMsg("Zone fans locked.");
        record(recordMsg);
        locked = true;
        for (auto& fan : _fans)
        {
            zone.lockFanTarget(fan, _target);
        }
    }
}

void OverrideFanTarget::setCount(const json& jsonObj)
{
    if (!jsonObj.contains("count"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required count value"};
    }
    _count = jsonObj["count"].get<size_t>();
}

void OverrideFanTarget::setState(const json& jsonObj)
{
    if (!jsonObj.contains("state"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required state value"};
    }
    _state = getJsonValue(jsonObj["state"]);
}

void OverrideFanTarget::setTarget(const json& jsonObj)
{
    if (!jsonObj.contains("target"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required target value"};
    }
    _target = jsonObj["target"].get<uint64_t>();
}

} // namespace phosphor::fan::control::json
