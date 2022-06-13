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
#include "init.hpp"

#include "../manager.hpp"
#include "action.hpp"
#include "group.hpp"
#include "sdbusplus.hpp"
#include "trigger_aliases.hpp"

#include <fmt/format.h>

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

#include <algorithm>
#include <iterator>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

namespace phosphor::fan::control::json::trigger::init
{

using json = nlohmann::json;
using namespace phosphor::logging;

void getProperties(Manager* mgr, const Group& group)
{
    for (const auto& member : group.getMembers())
    {
        try
        {
            // Check if property already cached
            auto value = mgr->getProperty(member, group.getInterface(),
                                          group.getProperty());
            if (value == std::nullopt)
            {
                // Property not in cache, attempt to add it
                mgr->addObjects(member, group.getInterface(),
                                group.getProperty(), group.getService());

                // If the service was predefined for the group, then we know
                // all members are in the same service so the above addObjects
                // call would have already added every present member in the
                // group (assuming the service has an ObjectManager iface
                // which it should). So no need to continue.
                if (!group.getService().empty())
                {
                    break;
                }
            }
        }
        catch (const std::exception& e)
        {
            // Configured dbus object does not exist on dbus yet?
            // TODO How to handle this? Create timer to keep checking for
            // object/service to appear? When to stop checking?
        }
    }
}

void nameHasOwner(Manager* mgr, const Group& group)
{
    bool hasOwner = false;
    std::string lastName = "";
    for (const auto& member : group.getMembers())
    {
        std::string servName = "";
        auto intf = group.getInterface();
        try
        {
            servName = group.getService();
            if (servName.empty())
            {
                servName = mgr->getService(member, intf);
            }
            if (!servName.empty())
            {
                if (lastName != servName)
                {
                    // Member not provided by same service as last group member
                    lastName = servName;
                    hasOwner = util::SDBusPlus::callMethodAndRead<bool>(
                        mgr->getBus(), "org.freedesktop.DBus",
                        "/org/freedesktop/DBus", "org.freedesktop.DBus",
                        "NameHasOwner", servName);
                }
                // Update service name owner state of group object
                mgr->setOwner(member, servName, intf, hasOwner);
            }
            else
            {
                // Path and/or interface configured does not exist on dbus?
                // TODO How to handle this? Create timer to keep checking for
                // object/service to appear? When to stop checking?
                log<level::ERR>(
                    fmt::format(
                        "Unable to get service name for path {}, interface {}",
                        member, intf)
                        .c_str());
            }
        }
        catch (const util::DBusMethodError& dme)
        {
            if (!servName.empty())
            {
                // Failed to get service name owner state
                hasOwner = false;
                mgr->setOwner(member, servName, intf, hasOwner);
            }
            else
            {
                // Path and/or interface configured does not exist on dbus?
                // TODO How to handle this? Create timer to keep checking for
                // object/service to appear? When to stop checking?
                log<level::ERR>(fmt::format("Unable to get service({}) owner "
                                            "state for path {}, interface {}",
                                            servName, member, intf)
                                    .c_str());
                throw dme;
            }
        }
    }
}

enableTrigger triggerInit(const json& jsonObj, const std::string& /*eventName*/,
                          std::vector<std::unique_ptr<ActionBase>>& /*actions*/)
{
    // Get the method handler if configured
    auto handler = methods.end();
    if (jsonObj.contains("method"))
    {
        auto method = jsonObj["method"].get<std::string>();
        std::transform(method.begin(), method.end(), method.begin(), tolower);
        handler = methods.find(method);
    }

    return [handler = std::move(handler)](
               const std::string& eventName, Manager* mgr,
               const std::vector<Group>& groups,
               std::vector<std::unique_ptr<ActionBase>>& actions) {
        // Event groups are optional, so a method is only required if there
        // are event groups i.e.) An init triggered event without any event
        // groups results in just running the actions
        if (!groups.empty() && handler == methods.end())
        {
            // Construct list of available methods
            auto availMethods = std::accumulate(
                std::next(methods.begin()), methods.end(),
                methods.begin()->first, [](auto list, auto method) {
                    return std::move(list) + ", " + method.first;
                });
            auto msg =
                fmt::format("Event '{}' requires a supported method given to "
                            "be init driven, available methods: {}",
                            eventName, availMethods);
            log<level::ERR>(msg.c_str());
            throw std::runtime_error(msg.c_str());
        }

        for (const auto& group : groups)
        {
            // Call method handler for each group to populate cache
            handler->second(mgr, group);
        }
        for (auto& action : actions)
        {
            // Run each action after initializing all the groups
            action->run();
        }
    };
}

} // namespace phosphor::fan::control::json::trigger::init
