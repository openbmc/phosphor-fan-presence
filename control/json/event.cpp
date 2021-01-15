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
#include "json_parser.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using namespace phosphor::logging;

const std::map<configKey, std::unique_ptr<Group>> Event::_availGrps =
    getConfig<Group>(true);

Event::Event(sdbusplus::bus::bus& bus, const json& jsonObj) :
    ConfigBase(jsonObj), _bus(bus)
{
    if (jsonObj.contains("profiles"))
    {
        for (const auto& profile : jsonObj["profiles"])
        {
            _profiles.emplace_back(profile.get<std::string>());
        }
    }

    // Event could have a precondition
    if (!jsonObj.contains("precondition"))
    {
        // Event groups are optional
        if (jsonObj.contains("groups"))
        {
            setGroups(jsonObj);
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
        setPrecond(jsonObj);
    }
}

void Event::setPrecond(const json& jsonObj)
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
    setGroups(precond);
    setTriggers(precond);
}

void Event::setGroups(const json& jsonObj)
{
    for (const auto& group : jsonObj["groups"])
    {
        if (!group.contains("name") || !group.contains("interface") ||
            !group.contains("property"))
        {
            log<level::ERR>("Missing required event group attributes",
                            entry("JSON=%s", group.dump().c_str()));
            throw std::runtime_error("Missing required event group attributes");
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
