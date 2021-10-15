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
#include "signal.hpp"

#include "../manager.hpp"
#include "action.hpp"
#include "group.hpp"
#include "handlers.hpp"
#include "trigger_aliases.hpp"

#include <fmt/format.h>

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus/match.hpp>

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

namespace phosphor::fan::control::json::trigger::signal
{

using json = nlohmann::json;
using namespace phosphor::logging;
using namespace sdbusplus::bus::match;

void subscribe(const std::string& match, SignalPkg&& signalPkg,
               std::function<bool(SignalPkg&)> isSameSig, Manager* mgr)
{
    auto& signalData = mgr->getSignal(match);
    if (signalData.empty())
    {
        // Signal subscription doesnt exist, add signal package and subscribe
        std::vector<SignalPkg> pkgs = {signalPkg};
        std::vector<SignalPkg> dataPkgs =
            std::vector<SignalPkg>(std::move(pkgs));
        std::unique_ptr<sdbusplus::server::match::match> ptrMatch = nullptr;
        // TODO(ibm-openbmc/#3195) - Filter signal subscriptions to objects
        // owned by fan control?
        if (!match.empty())
        {
            // Subscribe to signal
            ptrMatch = std::make_unique<sdbusplus::server::match::match>(
                mgr->getBus(), match.c_str(),
                std::bind(std::mem_fn(&Manager::handleSignal), &(*mgr),
                          std::placeholders::_1, dataPkgs));
        }
        signalData.emplace_back(std::move(dataPkgs), std::move(ptrMatch));
    }
    else
    {
        // Signal subscription already exists
        // Only a single signal data entry tied to each match is supported
        auto& pkgs = std::get<std::vector<SignalPkg>>(signalData.front());
        for (auto& pkg : pkgs)
        {
            if (isSameSig(pkg))
            {
                // Same signal expected, add actions to be run when signal
                // received
                auto& pkgActions = std::get<SignalActions>(signalPkg);
                auto& actions = std::get<SignalActions>(pkg);
                actions.insert(actions.end(),
                               std::make_move_iterator(pkgActions.begin()),
                               std::make_move_iterator(pkgActions.end()));
            }
            else
            {
                // Expected signal differs, add signal package
                pkgs.emplace_back(std::move(signalPkg));
            }
        }
    }
}

void propertiesChanged(Manager* mgr, const Group& group, SignalActions actions,
                       const json&)
{
    // Groups are optional, but a signal triggered event with no groups
    // will do nothing since signals require a group
    for (const auto& member : group.getMembers())
    {
        // Setup property changed signal handler on the group member's
        // property
        const auto match =
            rules::propertiesChanged(member, group.getInterface());
        SignalPkg signalPkg = {Handlers::propertiesChanged,
                               SignalObject(std::cref(member),
                                            std::cref(group.getInterface()),
                                            std::cref(group.getProperty())),
                               SignalActions(actions)};
        auto isSameSig = [&prop = group.getProperty()](SignalPkg& pkg) {
            auto& obj = std::get<SignalObject>(pkg);
            return prop == std::get<Prop>(obj);
        };

        subscribe(match, std::move(signalPkg), isSameSig, mgr);
    }
}

void interfacesAdded(Manager* mgr, const Group& group, SignalActions actions,
                     const json&)
{
    // Groups are optional, but a signal triggered event with no groups
    // will do nothing since signals require a group
    for (const auto& member : group.getMembers())
    {
        // Setup interfaces added signal handler on the group member
        const auto match = rules::interfacesAdded(member);
        SignalPkg signalPkg = {Handlers::interfacesAdded,
                               SignalObject(std::cref(member),
                                            std::cref(group.getInterface()),
                                            std::cref(group.getProperty())),
                               SignalActions(actions)};
        auto isSameSig = [&intf = group.getInterface()](SignalPkg& pkg) {
            auto& obj = std::get<SignalObject>(pkg);
            return intf == std::get<Intf>(obj);
        };

        subscribe(match, std::move(signalPkg), isSameSig, mgr);
    }
}

void interfacesRemoved(Manager* mgr, const Group& group, SignalActions actions,
                       const json&)
{
    // Groups are optional, but a signal triggered event with no groups
    // will do nothing since signals require a group
    for (const auto& member : group.getMembers())
    {
        // Setup interfaces added signal handler on the group member
        const auto match = rules::interfacesRemoved(member);
        SignalPkg signalPkg = {Handlers::interfacesRemoved,
                               SignalObject(std::cref(member),
                                            std::cref(group.getInterface()),
                                            std::cref(group.getProperty())),
                               SignalActions(actions)};
        auto isSameSig = [&intf = group.getInterface()](SignalPkg& pkg) {
            auto& obj = std::get<SignalObject>(pkg);
            return intf == std::get<Intf>(obj);
        };

        subscribe(match, std::move(signalPkg), isSameSig, mgr);
    }
}

void nameOwnerChanged(Manager* mgr, const Group& group, SignalActions actions,
                      const json&)
{
    // Groups are optional, but a signal triggered event with no groups
    // will do nothing since signals require a group
    for (const auto& member : group.getMembers())
    {
        auto serv = Manager::getService(member, group.getInterface());
        if (!serv.empty())
        {
            // Setup name owner changed signal handler on the group
            // member's service
            const auto match = rules::nameOwnerChanged(serv);
            SignalPkg signalPkg = {Handlers::nameOwnerChanged,
                                   SignalObject(std::cref(member),
                                                std::cref(group.getInterface()),
                                                std::cref(group.getProperty())),
                                   SignalActions(actions)};
            // If signal match already exists, then the service will be the
            // same so add action to be run
            auto isSameSig = [](SignalPkg& pkg) { return true; };

            subscribe(match, std::move(signalPkg), isSameSig, mgr);
        }
        else
        {
            // Unable to construct nameOwnerChanged match string
            // Path and/or interface configured does not exist on dbus yet?
            // TODO How to handle this? Create timer to keep checking for
            // service to appear? When to stop checking?
            log<level::ERR>(
                fmt::format("Events will not be triggered by name owner changed"
                            "signals from service of path {}, interface {}",
                            member, group.getInterface())
                    .c_str());
        }
    }
}

void member(Manager* mgr, const Group& group, SignalActions actions,
            const json&)
{
    // No SignalObject required to associate to this signal
    SignalPkg signalPkg = {Handlers::member, SignalObject(),
                           SignalActions(actions)};
    // If signal match already exists, then the member signal will be
    // the same so add action to be run
    auto isSameSig = [](SignalPkg& pkg) { return true; };

    // Groups are optional, but a signal triggered event with no groups
    // will do nothing since signals require a group
    for (const auto& member : group.getMembers())
    {
        // Subscribe for signal from each group member
        const auto match =
            rules::type::signal() + rules::member(group.getProperty()) +
            rules::path(member) + rules::interface(group.getInterface());

        subscribe(match, std::move(signalPkg), isSameSig, mgr);
    }
}

enableTrigger triggerSignal(const json& jsonObj, const std::string& eventName,
                            SignalActions actions)
{
    auto subscriber = signals.end();
    if (jsonObj.contains("signal"))
    {
        auto signal = jsonObj["signal"].get<std::string>();
        std::transform(signal.begin(), signal.end(), signal.begin(), tolower);
        subscriber = signals.find(signal);
    }
    if (subscriber == signals.end())
    {
        // Construct list of available signals
        auto availSignals =
            std::accumulate(std::next(signals.begin()), signals.end(),
                            signals.begin()->first, [](auto list, auto signal) {
                                return std::move(list) + ", " + signal.first;
                            });
        auto msg =
            fmt::format("Event '{}' requires a supported signal given to be "
                        "triggered by signal, available signals: {}",
                        eventName, availSignals);
        log<level::ERR>(msg.c_str());
        throw std::runtime_error(msg.c_str());
    }

    return [subscriber = std::move(subscriber),
            jsonObj](const std::string& eventName, Manager* mgr,
                     const std::vector<Group>& groups,
                     std::vector<std::unique_ptr<ActionBase>>& actions) {
        for (const auto& group : groups)
        {
            // Call signal subscriber for each group
            subscriber->second(mgr, group, actions, jsonObj);
        }
    };
}

} // namespace phosphor::fan::control::json::trigger::signal
