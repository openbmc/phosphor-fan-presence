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
#include "sdbusplus.hpp"
#include "triggers/trigger.hpp"

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

Event::Event(const json& jsonObj, Manager* mgr,
             std::map<configKey, std::unique_ptr<Zone>>& zones) :
    ConfigBase(jsonObj),
    _bus(util::SDBusPlus::getBus()), _manager(mgr), _zones(zones)
{
    // Event could have a precondition
    if (!jsonObj.contains("precondition"))
    {
        // Event groups are optional
        if (jsonObj.contains("groups"))
        {
            setGroups(jsonObj, _groups);
        }
        // Event actions are optional
        if (jsonObj.contains("actions"))
        {
            setActions(jsonObj);
        }
        setTriggers(jsonObj);
    }
    else
    {
        setPrecond(jsonObj);
    }
}

auto& Event::getAvailGroups()
{
    static auto groups = Manager::getConfig<Group>(true);
    return groups;
}

void Event::configGroup(Group& group, const json& jsonObj)
{
    if (!jsonObj.contains("interface") || !jsonObj.contains("property") ||
        !jsonObj["property"].contains("name"))
    {
        log<level::ERR>("Missing required group attribute",
                        entry("JSON=%s", jsonObj.dump().c_str()));
        throw std::runtime_error("Missing required group attribute");
    }

    // Get the group members' interface
    auto intf = jsonObj["interface"].get<std::string>();
    group.setInterface(intf);

    // Get the group members' property name
    auto prop = jsonObj["property"]["name"].get<std::string>();
    group.setProperty(prop);

    // Get the group members' data type
    if (jsonObj["property"].contains("type"))
    {
        std::optional<std::string> type =
            jsonObj["property"]["type"].get<std::string>();
        group.setType(type);
    }

    // Get the group members' expected value
    if (jsonObj["property"].contains("value"))
    {
        std::optional<PropertyVariantType> value =
            getJsonValue(jsonObj["property"]["value"]);
        group.setValue(value);
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
    setGroups(precond, _groups);
    setTriggers(precond);
}

void Event::setGroups(const json& jsonObj, std::vector<Group>& groups)
{
    auto& availGroups = getAvailGroups();
    for (const auto& jsonGrp : jsonObj["groups"])
    {
        if (!jsonGrp.contains("name"))
        {
            auto msg = fmt::format(
                "Missing required group name attribute in event {}", getName());
            log<level::ERR>(msg.c_str(),
                            entry("JSON=%s", jsonGrp.dump().c_str()));
            throw std::runtime_error(msg.c_str());
        }

        configKey eventProfile =
            std::make_pair(jsonGrp["name"].get<std::string>(), _profiles);
        auto grpEntry =
            std::find_if(availGroups.begin(), availGroups.end(),
                         [&eventProfile](const auto& grp) {
                             return Manager::inConfig(grp.first, eventProfile);
                         });
        if (grpEntry != availGroups.end())
        {
            auto group = Group(*grpEntry->second);
            configGroup(group, jsonGrp);
            groups.emplace_back(group);
        }
    }
}

void Event::setActions(const json& jsonObj)
{
    for (const auto& jsonAct : jsonObj["actions"])
    {
        if (!jsonAct.contains("name"))
        {
            log<level::ERR>("Missing required event action name",
                            entry("JSON=%s", jsonAct.dump().c_str()));
            throw std::runtime_error("Missing required event action name");
        }

        // Append action specific groups to the list of event groups for each
        // action in the event
        auto actionGroups = _groups;
        if (jsonAct.contains("groups"))
        {
            setGroups(jsonAct, actionGroups);
        }
        if (actionGroups.empty())
        {
            log<level::DEBUG>(
                fmt::format("No groups configured for event {}'s action {} "
                            "based on the active profile(s)",
                            getName(), jsonAct["name"].get<std::string>())
                    .c_str());
        }

        // Determine list of zones action should be run against
        std::vector<std::reference_wrapper<Zone>> actionZones;
        if (!jsonAct.contains("zones"))
        {
            // No zones configured on the action results in the action running
            // against all zones matching the event's active profiles
            for (const auto& zone : _zones)
            {
                configKey eventProfile =
                    std::make_pair(zone.second->getName(), _profiles);
                auto zoneEntry = std::find_if(_zones.begin(), _zones.end(),
                                              [&eventProfile](const auto& z) {
                                                  return Manager::inConfig(
                                                      z.first, eventProfile);
                                              });
                if (zoneEntry != _zones.end())
                {
                    actionZones.emplace_back(*zoneEntry->second);
                }
            }
        }
        else
        {
            // Zones configured on the action result in the action only running
            // against those zones if they match the event's active profiles
            for (const auto& jsonZone : jsonAct["zones"])
            {
                configKey eventProfile =
                    std::make_pair(jsonZone.get<std::string>(), _profiles);
                auto zoneEntry = std::find_if(_zones.begin(), _zones.end(),
                                              [&eventProfile](const auto& z) {
                                                  return Manager::inConfig(
                                                      z.first, eventProfile);
                                              });
                if (zoneEntry != _zones.end())
                {
                    actionZones.emplace_back(*zoneEntry->second);
                }
            }
        }
        if (actionZones.empty())
        {
            log<level::DEBUG>(
                fmt::format("No zones configured for event {}'s action {} "
                            "based on the active profile(s)",
                            getName(), jsonAct["name"].get<std::string>())
                    .c_str());
        }

        // Create the action for the event
        auto actObj = ActionFactory::getAction(
            jsonAct["name"].get<std::string>(), jsonAct,
            std::move(actionGroups), std::move(actionZones));
        if (actObj)
        {
            _actions.emplace_back(std::move(actObj));
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
    for (const auto& jsonTrig : jsonObj["triggers"])
    {
        if (!jsonTrig.contains("class"))
        {
            log<level::ERR>("Missing required event trigger class",
                            entry("JSON=%s", jsonTrig.dump().c_str()));
            throw std::runtime_error("Missing required event trigger class");
        }
        // The class of trigger used to run the event actions
        auto tClass = jsonTrig["class"].get<std::string>();
        std::transform(tClass.begin(), tClass.end(), tClass.begin(), tolower);
        auto trigFunc = trigger::triggers.find(tClass);
        if (trigFunc != trigger::triggers.end())
        {
            trigFunc->second(jsonTrig, getName(), _manager, _actions);
        }
        else
        {
            // Construct list of available triggers
            auto availTrigs = std::accumulate(
                std::next(trigger::triggers.begin()), trigger::triggers.end(),
                trigger::triggers.begin()->first, [](auto list, auto trig) {
                    return std::move(list) + ", " + trig.first;
                });
            log<level::ERR>(
                fmt::format("Trigger '{}' is not recognized", tClass).c_str(),
                entry("AVAILABLE_TRIGGERS=%s", availTrigs.c_str()));
            throw std::runtime_error("Unsupported trigger class name given");
        }
    }
}

} // namespace phosphor::fan::control::json
