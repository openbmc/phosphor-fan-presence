/**
 * Copyright © 2026 IBM Corporation
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
#include "chassis_manager.hpp"

#include "sdbusplus.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus/match.hpp>

#include <map>
#include <string>
#include <variant>

namespace phosphor::fan::control::json
{

ChassisManager& ChassisManager::instance()
{
    static ChassisManager mgr;
    return mgr;
}

void ChassisManager::init(
    sdbusplus::bus_t& bus,
    std::function<void(const std::string&)> onChassisStatusChanged)
{
    _bus = &bus;
    _onChassisStatusChanged = std::move(onChassisStatusChanged);
    _chassis.clear();
    _propMatches.clear();
    _ifaceMatches.clear();
}

void ChassisManager::registerChassis(const std::string& path,
                                     bool checkAvailability)
{
    auto [it, inserted] = _chassis.emplace(path, ChassisState{});
    auto& state = it->second;
    state.path = path;
    state.checkAvailability = checkAvailability;

    try
    {
        state.present = util::SDBusPlus::getProperty<bool>(
            *_bus, path, INV_ITEM_IFACE, "Present");
        state.presentIfaceSeen = true;
    }
    catch (const util::DBusServiceError&)
    {
        lg2::debug(
            "ChassisManager: {PATH} Inventory.Item not on D-Bus yet, waiting",
            "PATH", path);
    }
    catch (const util::DBusPropertyError&)
    {
        // Interface exists but no Present property - unexpected; treat as
        // present so we don't block non-multi-chassis systems.
        state.present = true;
        state.presentIfaceSeen = true;
    }

    if (checkAvailability)
    {
        try
        {
            state.available = util::SDBusPlus::getProperty<bool>(
                *_bus, path, AVAILABILITY_INTF, "Available");
            state.availIfaceSeen = true;
        }
        catch (const util::DBusServiceError&)
        {
            lg2::debug(
                "ChassisManager: {PATH} Availability not on D-Bus yet, waiting",
                "PATH", path);
        }
        catch (const util::DBusPropertyError&)
        {
            // Path exists but no Availability interface yet; wait
        }
    }

    if (checkAvailability)
    {
        lg2::info(
            "ChassisManager: registered {PATH} present={PRESENT} available={AVAIL}",
            "PATH", path, "PRESENT", state.present, "AVAIL", state.available);
    }
    else
    {
        lg2::info("ChassisManager: registered {PATH} present={PRESENT}", "PATH",
                  path, "PRESENT", state.present);
    }

    if (!inserted)
    {
        // inserted == true -> a new entry was created, continue to subscribe
        // inserted == false -> nothing was inserted, return
        return;
    }

    // Subscribe to propertiesChanged for Inventory.Item (Present)
    _propMatches.emplace_back(
        *_bus, sdbusplus::match_rules::propertiesChanged(path, INV_ITEM_IFACE),
        [this, path](sdbusplus::message_t& msg) {
            propertiesChanged(path, msg);
        });

    // Subscribe to propertiesChanged for Availability (if used)
    if (checkAvailability)
    {
        _propMatches.emplace_back(
            *_bus,
            sdbusplus::match_rules::propertiesChanged(path, AVAILABILITY_INTF),
            [this, path](sdbusplus::message_t& msg) {
                propertiesChanged(path, msg);
            });
    }

    // Subscribe to interfacesAdded for the path
    _ifaceMatches.emplace_back(
        *_bus, sdbusplus::match_rules::interfacesAddedAtPath(path),
        [this, path](sdbusplus::message_t& msg) {
            interfacesAdded(path, msg);
        });
}

bool ChassisManager::isReady(const std::string& path) const
{
    if (path.empty())
    {
        // Not a multi-chassis fan - always ready
        // Backwards compatability for fans.json that don't have the
        // "chassis_path" variable
        return true;
    }
    auto it = _chassis.find(path);
    if (it == _chassis.end())
    {
        // Path never registered (should not happen, but treat as ready)
        return true;
    }
    return it->second.isReady();
}

void ChassisManager::propertiesChanged(const std::string& path,
                                       sdbusplus::message_t& msg)
{
    auto it = _chassis.find(path);
    if (it == _chassis.end())
    {
        return;
    }
    auto& state = it->second;

    auto [iface, props] =
        msg.unpack<std::string, std::map<std::string, std::variant<bool>>>();

    bool changed = false;

    if (iface == INV_ITEM_IFACE)
    {
        auto p = props.find("Present");
        if (p != props.end())
        {
            bool newVal = std::get<bool>(p->second);
            if (state.present != newVal || !state.presentIfaceSeen)
            {
                state.present = newVal;
                state.presentIfaceSeen = true;
                changed = true;
                lg2::info("ChassisManager: {PATH} Present changed to {VAL}",
                          "PATH", path, "VAL", newVal);
            }
        }
    }
    else if (iface == AVAILABILITY_INTF && state.checkAvailability)
    {
        auto p = props.find("Available");
        if (p != props.end())
        {
            bool newVal = std::get<bool>(p->second);
            if (state.available != newVal || !state.availIfaceSeen)
            {
                state.available = newVal;
                state.availIfaceSeen = true;
                changed = true;
                lg2::info("ChassisManager: {PATH} Available changed to {VAL}",
                          "PATH", path, "VAL", newVal);
            }
        }
    }

    if (changed && _onChassisStatusChanged)
    {
        _onChassisStatusChanged(path);
    }
}

void ChassisManager::interfacesAdded(const std::string& path,
                                     sdbusplus::message_t& msg)
{
    auto it = _chassis.find(path);
    if (it == _chassis.end())
    {
        return;
    }
    auto& state = it->second;

    auto [objPath, ifaces] = msg.unpack<
        sdbusplus::object_path,
        std::map<std::string,
                 std::map<std::string, std::variant<bool, std::string, int64_t,
                                                    double, uint64_t>>>>();

    bool changed = false;

    auto invIt = ifaces.find(INV_ITEM_IFACE);
    if (invIt != ifaces.end())
    {
        auto p = invIt->second.find("Present");
        if (p != invIt->second.end())
        {
            if (const bool* val = std::get_if<bool>(&p->second))
            {
                state.present = *val;
                state.presentIfaceSeen = true;
                changed = true;
                lg2::info(
                    "ChassisManager: {PATH} Present (iface added) = {VAL}",
                    "PATH", path, "VAL", *val);
            }
        }
    }

    if (state.checkAvailability)
    {
        auto avIt = ifaces.find(AVAILABILITY_INTF);
        if (avIt != ifaces.end())
        {
            auto p = avIt->second.find("Available");
            if (p != avIt->second.end())
            {
                if (const bool* val = std::get_if<bool>(&p->second))
                {
                    state.available = *val;
                    state.availIfaceSeen = true;
                    changed = true;
                    lg2::info(
                        "ChassisManager: {PATH} Available (iface added) = {VAL}",
                        "PATH", path, "VAL", *val);
                }
            }
        }
    }

    if (changed && _onChassisStatusChanged)
    {
        _onChassisStatusChanged(path);
    }
}

} // namespace phosphor::fan::control::json
