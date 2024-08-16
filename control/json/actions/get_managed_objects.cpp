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
#include "get_managed_objects.hpp"

#include "../manager.hpp"
#include "event.hpp"

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

GetManagedObjects::GetManagedObjects(const json& jsonObj,
                                     const std::vector<Group>& groups) :
    ActionBase(jsonObj, groups)
{
    setActions(jsonObj);
}

void GetManagedObjects::run(Zone& zone)
{
    std::set<std::string> services;

    // Call Manager::addObjects to refresh the values of the group members.
    // If there is an ObjectManager interface that handles them, then
    // the code can combine all members in the same service down to one call.
    // If no ObjectManager, then still need addObjects calls for each.
    for (const auto& group : _groups)
    {
        for (const auto& member : group.getMembers())
        {
            try
            {
                std::vector<std::string> objMgrPaths;

                const auto& service =
                    zone.getManager()->getService(member, group.getInterface());

                if (!service.empty())
                {
                    objMgrPaths = zone.getManager()->getPaths(
                        service, "org.freedesktop.DBus.ObjectManager");
                }
                else
                {
                    continue;
                }

                // Look for the ObjectManager as an ancestor of the path.
                auto hasObjMgr = std::any_of(
                    objMgrPaths.begin(), objMgrPaths.end(),
                    [member](const auto& path) {
                        return member.find(path) != std::string::npos;
                    });

                if (!hasObjMgr || services.find(service) == services.end())
                {
                    if (hasObjMgr)
                    {
                        services.insert(service);
                    }

                    zone.getManager()->addObjects(member, group.getInterface(),
                                                  group.getProperty());
                }
            }
            catch (const std::exception& e)
            {
                // May have been called from a name_owner_changed trigger
                // and the service may have been lost.
            }
        }
    }

    // Perform the actions
    std::for_each(_actions.begin(), _actions.end(),
                  [](auto& action) { action->run(); });
}

void GetManagedObjects::setZones(
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

void GetManagedObjects::setActions(const json& jsonObj)
{
    if (!jsonObj.contains("actions"))
    {
        return;
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
            profiles = jsonAct["profiles"].get<std::vector<std::string>>();
        }

        // Set the groups configured for each action run when the timer expires
        std::vector<Group> groups;
        Event::setGroups(jsonAct, profiles, groups);

        // If no groups on that action, use our own groups instead
        const std::vector<Group>* groupPtr = &groups;
        if (groups.empty())
        {
            groupPtr = &_groups;
        }

        // List of zones is set on these actions by overriden setZones()
        auto actObj = ActionFactory::getAction(
            jsonAct["name"].get<std::string>(), jsonAct, *groupPtr, {});
        if (actObj)
        {
            _actions.emplace_back(std::move(actObj));
        }
    }
}

} // namespace phosphor::fan::control::json
