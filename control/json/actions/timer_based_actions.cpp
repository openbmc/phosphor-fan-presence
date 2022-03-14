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
#include "timer_based_actions.hpp"

#include "../manager.hpp"
#include "action.hpp"
#include "event.hpp"
#include "group.hpp"
#include "sdbusplus.hpp"
#include "sdeventplus.hpp"
#include "zone.hpp"

#include <fmt/format.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

TimerBasedActions::TimerBasedActions(const json& jsonObj,
                                     const std::vector<Group>& groups) :
    ActionBase(jsonObj, groups),
    _timer(util::SDEventPlus::getEvent(),
           std::bind(&TimerBasedActions::timerExpired, this))
{
    // If any of groups' value == nullopt(i.e. not configured), action is
    // driven by the service owned state of the group members
    _byOwner =
        std::any_of(_groups.begin(), _groups.end(), [](const auto& group) {
            return group.getValue() == std::nullopt;
        });

    setTimerConf(jsonObj);
    setActions(jsonObj);
}

void TimerBasedActions::run(Zone& zone)
{
    if (_byOwner)
    {
        // If any service providing a group member is not owned, start
        // timer and if all members' services are owned, stop timer.
        if (std::any_of(_groups.begin(), _groups.end(), [](const auto& group) {
                const auto& members = group.getMembers();
                return std::any_of(members.begin(), members.end(),
                                   [&group](const auto& member) {
                                       return !Manager::hasOwner(
                                           member, group.getInterface());
                                   });
            }))
        {
            startTimer();
        }
        else
        {
            stopTimer();
        }
    }
    else
    {
        auto* mgr = zone.getManager();
        // If all group members have a given value and it matches what's
        // in the cache, start timer and if any do not match, stop
        // timer.
        if (std::all_of(
                _groups.begin(), _groups.end(), [&mgr](const auto& group) {
                    const auto& members = group.getMembers();
                    return std::all_of(members.begin(), members.end(),
                                       [&mgr, &group](const auto& member) {
                                           return group.getValue() ==
                                                  mgr->getProperty(
                                                      member,
                                                      group.getInterface(),
                                                      group.getProperty());
                                       });
                }))
        {
            // Timer will be started(and never stopped) when _groups is empty
            startTimer();
        }
        else
        {
            stopTimer();
        }
    }
}

void TimerBasedActions::startTimer()
{
    if (!_timer.isEnabled())
    {
        if (_type == TimerType::repeating)
        {
            _timer.restart(_interval);
        }
        else if (_type == TimerType::oneshot)
        {
            _timer.restartOnce(_interval);
        }
    }
}

void TimerBasedActions::stopTimer()
{
    if (_timer.isEnabled())
    {
        _timer.setEnabled(false);
    }
    else
    {
        // Perform the actions in case state changed after the configured time
        std::for_each(_actions.begin(), _actions.end(),
                      [](auto& action) { action->run(); });
    }
}

void TimerBasedActions::timerExpired()
{
    // Perform the actions
    std::for_each(_actions.begin(), _actions.end(),
                  [](auto& action) { action->run(); });
}

void TimerBasedActions::setZones(
    std::vector<std::reference_wrapper<Zone>>& zones)
{
    for (auto& zone : zones)
    {
        this->addZone(zone);
        // Add zone to _actions
        std::for_each(_actions.begin(), _actions.end(),
                      [&zone](std::unique_ptr<ActionBase>& action) {
                          action->addZone(zone);
                      });
    }
}

void TimerBasedActions::setTimerConf(const json& jsonObj)
{
    if (!jsonObj.contains("timer"))
    {
        throw ActionParseError{getName(), "Missing required timer entry"};
    }
    auto jsonTimer = jsonObj["timer"];
    if (!jsonTimer.contains("interval") || !jsonTimer.contains("type"))
    {
        throw ActionParseError{
            getName(), "Missing required timer parameters {interval, type}"};
    }

    // Interval provided in microseconds
    _interval = static_cast<std::chrono::microseconds>(
        jsonTimer["interval"].get<uint64_t>());

    // Retrieve type of timer
    auto type = jsonTimer["type"].get<std::string>();
    if (type == "oneshot")
    {
        _type = TimerType::oneshot;
    }
    else if (type == "repeating")
    {
        _type = TimerType::repeating;
    }
    else
    {
        throw ActionParseError{
            getName(), fmt::format("Timer type '{}' is not supported", type)};
    }
}

void TimerBasedActions::setActions(const json& jsonObj)
{
    if (!jsonObj.contains("actions"))
    {
        throw ActionParseError{getName(), "Missing required actions entry"};
    }
    for (const auto& jsonAct : jsonObj["actions"])
    {
        if (!jsonAct.contains("name"))
        {
            throw ActionParseError{getName(), "Missing required action name"};
        }

        // Get any configured profile restrictions on the action
        std::vector<std::string> profiles;
        if (jsonAct.contains("profiles"))
        {
            for (const auto& profile : jsonAct["profiles"])
            {
                profiles.emplace_back(profile.get<std::string>());
            }
        }

        // Set the groups configured for each action run when the timer expires
        std::vector<Group> groups;
        Event::setGroups(jsonAct, profiles, groups);

        // List of zones is set on these actions by overriden setZones()
        auto actObj = ActionFactory::getAction(
            jsonAct["name"].get<std::string>(), jsonAct, std::move(groups), {});
        if (actObj)
        {
            _actions.emplace_back(std::move(actObj));
        }
    }
}

} // namespace phosphor::fan::control::json
