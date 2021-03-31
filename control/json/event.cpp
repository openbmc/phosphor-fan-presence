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

#include "action.hpp"
#include "config_base.hpp"
#include "group.hpp"
#include "manager.hpp"

#include <fmt/format.h>

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

#include <algorithm>
#include <optional>

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

        // Get the group members' interface
        auto intf = group["interface"].get<std::string>();

        // Get the group members' property name
        auto prop = group["property"]["name"].get<std::string>();

        // Get the group members' data type
        std::optional<std::string> type = std::nullopt;
        if (group["property"].contains("type"))
        {
            type = group["property"]["type"].get<std::string>();
        }

        // Get the group members' expected value
        std::optional<PropertyVariantType> value = std::nullopt;
        if (group["property"].contains("value"))
        {
            value = getJsonValue(group["property"]["value"]);
        }

        configKey eventProfile =
            std::make_pair(group["name"].get<std::string>(), _profiles);
        auto grpEntry = std::find_if(
            groups.begin(), groups.end(), [&eventProfile](const auto& grp) {
                return Manager::inConfig(grp.first, eventProfile);
            });
        if (grpEntry != groups.end())
        {
            auto grp = Group(*grpEntry->second);
            grp.setInterface(intf);
            grp.setProperty(prop);
            grp.setType(type);
            grp.setValue(value);
            _groups.emplace_back(grp);
        }
    }

    if (_groups.empty())
    {
        log<level::ERR>(
            fmt::format(
                "No groups configured for event {} in its active profile(s)",
                getName())
                .c_str());
        throw std::runtime_error(
            fmt::format(
                "No groups configured for event {} in its active profile(s)",
                getName())
                .c_str());
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
        auto actObj =
            ActionFactory::getAction(action["name"].get<std::string>(), action);
        if (actObj)
        {
            _actions.emplace_back(std::move(actObj));
        }
    }
}

} // namespace phosphor::fan::control::json
