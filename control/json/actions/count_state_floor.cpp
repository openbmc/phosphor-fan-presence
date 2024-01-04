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
#include "sdeventplus.hpp"

namespace phosphor::fan::control::json
{

CountStateFloor::CountStateFloor(const json& jsonObj,
                                 const std::vector<Group>& groups) :
    ActionBase(jsonObj, groups)
{
    setCount(jsonObj);
    setState(jsonObj);
    setFloor(jsonObj);
    setDelayTime(jsonObj);
}

void CountStateFloor::run(Zone& zone)
{
    auto countReached = doCount();

    if (_delayTime == std::chrono::seconds::zero())
    {
        // If no delay time configured, can immediately update the hold.
        zone.setFloorHold(getUniqueName(), _floor, countReached);
        return;
    }

    if (!countReached)
    {
        if (_timer && _timer->isEnabled())
        {
            record("Stopping delay timer");
            _timer->setEnabled(false);
        }

        zone.setFloorHold(getUniqueName(), _floor, countReached);
        return;
    }

    // The count has been reached and a delay is configured, so either:
    // 1. This hold has already been set, so don't need to do anything else.
    // 2. The timer hasn't been started yet, so start it (May need to create
    //    it first).
    // 3. The timer is already running, don't need to do anything else.
    // When the timer expires, then count again and set the hold.

    if (zone.hasFloorHold(getUniqueName()))
    {
        return;
    }

    if (!_timer)
    {
        _timer = std::make_unique<Timer>(util::SDEventPlus::getEvent(),
                                         [&zone, this](Timer&) {
            zone.setFloorHold(getUniqueName(), _floor, doCount());
        });
    }

    if (!_timer->isEnabled())
    {
        record("Starting delay timer");
        _timer->restartOnce(_delayTime);
    }
}

bool CountStateFloor::doCount()
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
                        return true;
                    }
                }
            }
            catch (const std::out_of_range& oore)
            {
                // Default to property not equal when not found
            }
        }
    }

    return false;
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

void CountStateFloor::setDelayTime(const json& jsonObj)
{
    if (jsonObj.contains("delay"))
    {
        _delayTime = std::chrono::seconds(jsonObj["delay"].get<size_t>());
    }
}

} // namespace phosphor::fan::control::json
