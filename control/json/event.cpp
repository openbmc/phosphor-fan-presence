/**
 * Copyright © 2020 IBM Corporation
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
#include "event.hpp"

#include "group.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

#include <optional>
#include <tuple>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using namespace phosphor::logging;

Event::Event(const json& jsonObj, sdbusplus::bus::bus& bus,
             std::map<configKey, std::unique_ptr<Group>>& groups) :
    ConfigBase(jsonObj),
    _bus(bus)
{
    // Event could have a precondition
    if (!jsonObj.contains("precondition"))
    {
        // Event groups are optional
        if (jsonObj.contains("groups"))
        {
            setGroups(jsonObj, groups);
        }
        setTriggers(jsonObj);
        // Event actions are optional
        if (jsonObj.contains("actions"))
        {
            setActions(jsonObj);
        }
    }
    else
    {
        setPrecond(jsonObj, groups);
    }
}

void Event::setPrecond(const json& jsonObj,
                       std::map<configKey, std::unique_ptr<Group>>& groups)
{
    const auto& precond = jsonObj["precondition"];
    if (!precond.contains("name") || !precond.contains("groups") ||
        !precond.contains("triggers") || !precond.contains("events"))
    {
        log<level::ERR>("Missing required event precondition attributes",
                        entry("JSON=%s", precond.dump().c_str()));
        throw std::runtime_error(
            "Missing required event precondition attributes");
    }
    setGroups(precond, groups);
    setTriggers(precond);
}

void Event::setGroups(const json& jsonObj,
                      std::map<configKey, std::unique_ptr<Group>>& groups)
{
    for (const auto& group : jsonObj["groups"])
    {
        if (!group.contains("name") || !group.contains("interface") ||
            !group.contains("property") || !group["property"].contains("name"))
        {
            log<level::ERR>("Missing required event group attributes",
                            entry("JSON=%s", group.dump().c_str()));
            throw std::runtime_error("Missing required event group attributes");
        }

        // Get the group memebers' data type
        std::optional<std::string> type = std::nullopt;
        if (group["property"].contains("type"))
        {
            type = group["property"]["type"].get<std::string>();
        }

        // Get the group memebers' expected value
        std::optional<PropertyVariantType> value = std::nullopt;
        if (group["property"].contains("value"))
        {
            value = getJsonValue(group["property"]["value"]);
        }

        // Groups with the same profiles as the event can be used
        configKey key =
            std::make_pair(group["name"].get<std::string>(), _profiles);
        auto grpEntry = groups.find(key);
        if (grpEntry != groups.end())
        {
            eGroup grp;
            for (const auto& member : grpEntry->second->getMembers())
            {
                grp.emplace_back(std::make_tuple(
                    member, group["interface"].get<std::string>(),
                    group["property"]["name"].get<std::string>(), type, value));
            }
            _groups.emplace_back(grp);
        }
        else
        {
            // Groups with no profiles specified can be used in any event
            key = std::make_pair(group["name"].get<std::string>(),
                                 std::vector<std::string>{});
            grpEntry = groups.find(key);
            if (grpEntry != groups.end())
            {
                eGroup grp;
                for (const auto& member : grpEntry->second->getMembers())
                {
                    grp.emplace_back(std::make_tuple(
                        member, group["interface"].get<std::string>(),
                        group["property"]["name"].get<std::string>(), type,
                        value));
                }
                _groups.emplace_back(grp);
            }
        }
    }
}

void Event::setTriggers(const json& jsonObj)
{
    if (!jsonObj.contains("triggers"))
    {
        log<level::ERR>("Missing required event triggers list",
                        entry("JSON=%s", jsonObj.dump().c_str()));
        throw std::runtime_error("Missing required event triggers list");
    }
}

void Event::setActions(const json& jsonObj)
{
    for (const auto& action : jsonObj["actions"])
    {
        if (!action.contains("name"))
        {
            log<level::ERR>("Missing required event action name",
                            entry("JSON=%s", action.dump().c_str()));
            throw std::runtime_error("Missing required event action name");
        }
    }
}

} // namespace phosphor::fan::control::json
