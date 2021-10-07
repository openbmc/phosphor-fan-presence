/**
 * Copyright © 2022 IBM Corporation
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

OverrideFanTarget::OverrideFanTarget(const json& jsonObj,
                                     const std::vector<Group>& groups) :
    ActionBase(jsonObj, groups)
{
    setCount(jsonObj);
    setState(jsonObj);
    setTarget(jsonObj);
    setFans(jsonObj);
}

void OverrideFanTarget::run(Zone& zone)
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

                    if (numAtState >= _count)
                    {
                        break;
                    }
                }
            }
            catch (const std::out_of_range&)
            {}
        }

        // lock the fans
        if (numAtState >= _count)
        {
            lockFans(zone);
            break;
        }
    }

    if (_locked && numAtState < _count)
    {
        unlockFans(zone);
    }
}

void OverrideFanTarget::lockFans(Zone& zone)
{
    if (!_locked)
    {
        record("Adding fan target lock of " + std::to_string(_target) +
               " on zone " + zone.getName());

        for (auto& fan : _fans)
        {
            zone.lockFanTarget(fan, _target);
        }

        _locked = true;
    }
}

void OverrideFanTarget::unlockFans(Zone& zone)
{
    record("Un-locking fan target " + std::to_string(_target) + " on zone " +
           zone.getName());

    // unlock all fans in this instance
    for (auto& fan : _fans)
    {
        zone.unlockFanTarget(fan, _target);
    }

    _locked = false;
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

void OverrideFanTarget::setFans(const json& jsonObj)
{
    if (!jsonObj.contains("fans"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required fans value"};
    }

    _fans = jsonObj["fans"].get<std::vector<std::string>>();
}

} // namespace phosphor::fan::control::json
