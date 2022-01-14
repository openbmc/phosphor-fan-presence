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
#include "net_target_decrease.hpp"

#include "../manager.hpp"
#include "../zone.hpp"
#include "action.hpp"
#include "group.hpp"

#include <fmt/format.h>

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

#include <algorithm>
#include <variant>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using namespace phosphor::logging;

NetTargetDecrease::NetTargetDecrease(const json& jsonObj,
                                     const std::vector<Group>& groups) :
    ActionBase(jsonObj, groups)
{
    setState(jsonObj);
    setDelta(jsonObj);
}

void NetTargetDecrease::run(Zone& zone)
{
    if (!_stateParameter.empty())
    {
        auto s = Manager::getParameter(_stateParameter);
        if (!s)
        {
            return;
        }
        _state = *s;
    }

    auto netDelta = zone.getDecDelta();
    for (const auto& group : _groups)
    {
        for (const auto& member : group.getMembers())
        {
            try
            {
                auto value = Manager::getObjValueVariant(
                    member, group.getInterface(), group.getProperty());
                if (std::holds_alternative<int64_t>(value) ||
                    std::holds_alternative<double>(value))
                {
                    if (value >= _state)
                    {
                        // No decrease allowed for this group
                        netDelta = 0;
                        break;
                    }
                    else
                    {
                        // Decrease factor is the difference in configured state
                        // to the current value's state
                        uint64_t deltaFactor = 0;
                        if (auto dblPtr = std::get_if<double>(&value))
                        {
                            deltaFactor = static_cast<uint64_t>(
                                std::get<double>(_state) - *dblPtr);
                        }
                        else
                        {
                            deltaFactor = static_cast<uint64_t>(
                                std::get<int64_t>(_state) -
                                std::get<int64_t>(value));
                        }

                        // Multiply the decrease factor by the configured delta
                        // to get the net decrease delta for the given group
                        // member. The lowest net decrease delta of the entire
                        // group is the decrease requested.
                        if (netDelta == 0)
                        {
                            netDelta = deltaFactor * _delta;
                        }
                        else
                        {
                            netDelta = std::min(netDelta, deltaFactor * _delta);
                        }
                    }
                }
                else if (std::holds_alternative<bool>(value) ||
                         std::holds_alternative<std::string>(value))
                {
                    // Where a group of booleans or strings equal the state
                    // provided, request a decrease of the configured delta
                    if (_state == value)
                    {
                        if (netDelta == 0)
                        {
                            netDelta = _delta;
                        }
                        else
                        {
                            netDelta = std::min(netDelta, _delta);
                        }
                    }
                }
                else
                {
                    // Unsupported group member type for this action
                    log<level::ERR>(
                        fmt::format("Action {}: Unsupported group member type "
                                    "given. [object = {} : {} : {}]",
                                    ActionBase::getName(), member,
                                    group.getInterface(), group.getProperty())
                            .c_str());
                }
            }
            catch (const std::out_of_range& oore)
            {
                // Property value not found, netDelta unchanged
            }
        }
        // Update group's decrease allowed state
        zone.setDecreaseAllow(group.getName(), !(netDelta == 0));
    }
    // Request target decrease to occur on decrease interval
    zone.requestDecrease(netDelta);
}

void NetTargetDecrease::setState(const json& jsonObj)
{
    if (jsonObj.contains("state"))
    {
        _state = getJsonValue(jsonObj["state"]);
    }
    else if (jsonObj.contains("state_parameter_name"))
    {
        _stateParameter = jsonObj["state_parameter_name"].get<std::string>();
    }
    else
    {
        throw ActionParseError{
            ActionBase::getName(),
            "Missing required state or state_parameter_name value"};
    }
}

void NetTargetDecrease::setDelta(const json& jsonObj)
{
    if (!jsonObj.contains("delta"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required delta value"};
    }
    _delta = jsonObj["delta"].get<uint64_t>();
}

} // namespace phosphor::fan::control::json
