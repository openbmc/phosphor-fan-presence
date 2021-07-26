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
#include "net_target_increase.hpp"

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

NetTargetIncrease::NetTargetIncrease(const json& jsonObj,
                                     const std::vector<Group>& groups) :
    ActionBase(jsonObj, groups)
{
    setState(jsonObj);
    setDelta(jsonObj);
}

void NetTargetIncrease::run(Zone& zone)
{
    auto netDelta = zone.getIncDelta();
    PropertyVariantType stateValue;

    // If the state source group is present, get the state value
    // from that instead of _state.
    if (_stateSource)
    {
        try
        {
            // Only single member groups supported for now
            const auto& member = _stateSource->getMembers()[0];

            stateValue = Manager::getObjValueVariant(
                member, _stateSource->getInterface(),
                _stateSource->getProperty());
        }
        catch (std::out_of_range& oore)
        {
            log<level::DEBUG>(
                fmt::format(
                    "{}: Could not read state source property {} {} {}",
                    ActionBase::getName(), _stateSource->getMembers()[0],
                    _stateSource->getInterface(), _stateSource->getProperty())
                    .c_str());
            return;
        }

        if (_modifier)
        {
            try
            {
                stateValue = _modifier->doOp(stateValue);
            }
            catch (const std::exception& e)
            {
                log<level::DEBUG>(
                    fmt::format("{}: Could not perform modifier operation: {}",
                                ActionBase::getName(), e.what())
                        .c_str());
                return;
            }
        }
    }
    else
    {
        stateValue = _state;
    }

    for (const auto& group : _groups)
    {
        // The stateSource group isn't involved in this calculation.
        if (_stateSource && (_stateSource->getName() == group.getName()))
        {
            continue;
        }

        const auto& members = group.getMembers();
        std::for_each(
            members.begin(), members.end(),
            [this, &zone, &group, &netDelta, stateValue](const auto& member) {
                try
                {
                    auto value = Manager::getObjValueVariant(
                        member, group.getInterface(), group.getProperty());
                    if (std::holds_alternative<int64_t>(value) ||
                        std::holds_alternative<double>(value))
                    {
                        // Where a group of int/doubles are greater than or
                        // equal to the state(some value) provided, request an
                        // increase of the configured delta times the difference
                        // between the group member's value and configured state
                        // value.
                        if (value >= stateValue)
                        {
                            uint64_t incDelta = 0;
                            if (auto dblPtr = std::get_if<double>(&value))
                            {
                                incDelta = static_cast<uint64_t>(
                                    (*dblPtr - std::get<double>(stateValue)) *
                                    _delta);
                            }
                            else
                            {
                                // Increase by at least a single delta
                                // to attempt bringing under provided 'state'
                                auto deltaFactor =
                                    std::max((std::get<int64_t>(value) -
                                              std::get<int64_t>(stateValue)),
                                             1ll);
                                incDelta =
                                    static_cast<uint64_t>(deltaFactor * _delta);
                            }
                            netDelta = std::max(netDelta, incDelta);
                        }
                    }
                    else if (std::holds_alternative<bool>(value))
                    {
                        // Where a group of booleans equal the state(`true` or
                        // `false`) provided, request an increase of the
                        // configured delta
                        if (stateValue == value)
                        {
                            netDelta = std::max(netDelta, _delta);
                        }
                    }
                    else if (std::holds_alternative<std::string>(value))
                    {
                        // Where a group of strings equal the state(some string)
                        // provided, request an increase of the configured delta
                        if (stateValue == value)
                        {
                            netDelta = std::max(netDelta, _delta);
                        }
                    }
                    else
                    {
                        // Unsupported group member type for this action
                        log<level::ERR>(
                            fmt::format(
                                "Action {}: Unsupported group member type "
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
            });
    }
    // Request increase to target
    zone.requestIncrease(netDelta);
}

void NetTargetIncrease::setState(const json& jsonObj)
{
    if (jsonObj.contains("state"))
    {
        _state = getJsonValue(jsonObj["state"]);
    }
    else if (jsonObj.contains("state_source"))
    {
        const auto& source = jsonObj.at("state_source");
        if (!source.contains("group"))
        {
            log<level::ERR>("state_source entry in JSON missing 'group'");
            throw ActionParseError{ActionBase::getName(),
                                   "invalid state_source JSON"};
        }

        auto groupName = source["group"].get<std::string>();
        auto group = std::find_if(
            _groups.begin(), _groups.end(),
            [groupName](const auto& g) { return groupName == g.getName(); });

        if (group == _groups.end())
        {
            log<level::ERR>(
                fmt::format("Group name {} in state_source JSON is invalid",
                            groupName)
                    .c_str());
            throw ActionParseError{ActionBase::getName(),
                                   "invalid state_source JSON"};
        }

        _stateSource = &(*group);

        // Just support 1 member in the group for now.
        if (group->getMembers().size() != 1)
        {
            log<level::ERR>(
                fmt::format("Invalid number of members in group {}: {}",
                            group->getName(), group->getMembers().size())
                    .c_str());
            throw ActionParseError{ActionBase::getName(),
                                   "invalid state_source JSON"};
        }

        if (source.contains("modifier"))
        {
            try
            {
                _modifier = std::make_unique<Modifier>(source.at("modifier"));
            }
            catch (const std::invalid_argument& e)
            {
                throw ActionParseError{ActionBase::getName(), e.what()};
            }
        }
    }
    else
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required state or state_source value"};
    }
}

void NetTargetIncrease::setDelta(const json& jsonObj)
{
    if (!jsonObj.contains("delta"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required delta value"};
    }
    _delta = jsonObj["delta"].get<uint64_t>();
}

} // namespace phosphor::fan::control::json
