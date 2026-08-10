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

#include "../../utility.hpp"
#include "config_base.hpp"
#include "sdbusplus.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus/match.hpp>
#include <xyz/openbmc_project/Inventory/Item/common.hpp>
#include <xyz/openbmc_project/State/Decorator/Availability/common.hpp>

#include <algorithm>
#include <map>
#include <string>

namespace phosphor::fan::control::json
{

void ChassisManager::init(
    sdbusplus::bus_t& bus,
    std::function<void(const std::string&)> onChassisStatusChanged,
    std::function<void(const std::string&, const std::string&)>
        onFanSensorAppeared,
    std::function<void(const std::string&, const std::string&,
                       const std::string&)>
        onFanSensorLost)
{
    _bus = &bus;
    _onChassisStatusChanged = std::move(onChassisStatusChanged);
    _onFanSensorAppeared = std::move(onFanSensorAppeared);
    _onFanSensorLost = std::move(onFanSensorLost);
    _chassis.clear();
    _presentPropMatches.clear();
    _availPropMatches.clear();
    _ifaceAddedMatches.clear();
    _fanSensorWatches.clear();
    _fanSensorLossWatches.clear();
}

void ChassisManager::registerChassis(const std::string& path,
                                     bool checkAvailability)
{
    auto [it, inserted] = _chassis.emplace(path, ChassisState{});

    // Duplicate registration: no new subscriptions needed.
    if (!inserted)
    {
        return;
    }

    auto& state = it->second;
    state.checkAvailability = checkAvailability;

    try
    {
        state.present = util::SDBusPlus::getProperty<bool>(
            *_bus, path, util::INV_ITEM_IFACE, "Present");
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
    }

    if (checkAvailability)
    {
        try
        {
            state.available = util::SDBusPlus::getProperty<bool>(
                *_bus, path, AVAILABILITY_INTF, "Available");
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

        lg2::info("ChassisManager: registered {PATH} present={PRESENT} "
                  "available={AVAIL}",
                  "PATH", path, "PRESENT", state.present.value_or(false),
                  "AVAIL", state.available.value_or(false));
    }
    else
    {
        lg2::info("ChassisManager: registered {PATH} present={PRESENT}", "PATH",
                  path, "PRESENT", state.present.value_or(false));
    }

    // Subscribe to propertiesChanged for Inventory.Item (Present)
    _presentPropMatches.emplace_back(std::make_unique<sdbusplus::match>(
        *_bus,
        sdbusplus::match_rules::propertiesChanged(path, util::INV_ITEM_IFACE),
        std::bind_front(&ChassisManager::presentPropertyChanged, this, path)));

    if (checkAvailability)
    {
        // Subscribe to propertiesChanged for Availability
        _availPropMatches.emplace_back(std::make_unique<sdbusplus::match>(
            *_bus,
            sdbusplus::match_rules::propertiesChanged(path, AVAILABILITY_INTF),
            std::bind_front(&ChassisManager::availPropertyChanged, this,
                            path)));
    }

    // Single InterfacesAdded match handles both Inventory.Item and
    // Availability in one wakeup rather than two separate matches.
    _ifaceAddedMatches.emplace_back(std::make_unique<sdbusplus::match>(
        *_bus, sdbusplus::match_rules::interfacesAddedAtPath(path),
        std::bind_front(&ChassisManager::ifaceAdded, this, path)));
}

bool ChassisManager::isReady(const std::string& path) const
{
    if (path.empty())
    {
        // Not a multi-chassis fan - always ready.
        // Backwards compatibility for fans.json without "chassis_path".
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

void ChassisManager::presentPropertyChanged(const std::string& path,
                                            sdbusplus::message_t& msg)
{
    auto it = _chassis.find(path);
    if (it == _chassis.end())
    {
        return;
    }
    auto& state = it->second;

    using ItemVariant = sdbusplus::common::xyz::openbmc_project::inventory::
        Item::PropertiesVariant;
    auto [iface, props] =
        msg.unpack<std::string, std::map<std::string, ItemVariant>>();

    if (iface != util::INV_ITEM_IFACE)
    {
        return;
    }

    const auto p = props.find("Present");
    if (p == props.end())
    {
        return;
    }

    bool newVal = std::get<bool>(p->second);
    if (state.present != newVal)
    {
        state.present = newVal;
        lg2::info("ChassisManager: {PATH} Present changed to {VAL}", "PATH",
                  path, "VAL", newVal);
        if (_onChassisStatusChanged)
        {
            _onChassisStatusChanged(path);
        }
    }
}

void ChassisManager::availPropertyChanged(const std::string& path,
                                          sdbusplus::message_t& msg)
{
    auto it = _chassis.find(path);
    if (it == _chassis.end())
    {
        return;
    }
    auto& state = it->second;

    if (!state.checkAvailability)
    {
        return;
    }

    using AvailVariant = sdbusplus::common::xyz::openbmc_project::state::
        decorator::Availability::PropertiesVariant;
    auto [iface, props] =
        msg.unpack<std::string, std::map<std::string, AvailVariant>>();

    if (iface != AVAILABILITY_INTF)
    {
        return;
    }

    const auto p = props.find("Available");
    if (p == props.end())
    {
        return;
    }

    bool newVal = std::get<bool>(p->second);
    if (state.available != newVal)
    {
        state.available = newVal;
        lg2::info("ChassisManager: {PATH} Available changed to {VAL}", "PATH",
                  path, "VAL", newVal);
        if (_onChassisStatusChanged)
        {
            _onChassisStatusChanged(path);
        }
    }
}

void ChassisManager::ifaceAdded(const std::string& path,
                                sdbusplus::message_t& msg)
{
    auto it = _chassis.find(path);
    if (it == _chassis.end())
    {
        return;
    }
    auto& state = it->second;

    // InterfacesAdded carries a map of interface -> properties.  Use
    // PropertyVariantType (the union covering both Item and Availability
    // property types) so a single unpack covers both interfaces.
    using IfaceMap =
        std::map<std::string, std::map<std::string, PropertyVariantType>>;
    auto [objPath, ifaces] = msg.unpack<sdbusplus::object_path, IfaceMap>();

    bool changed = false;

    // Check Inventory.Item / Present
    auto invIt = ifaces.find(util::INV_ITEM_IFACE);
    if (invIt != ifaces.end())
    {
        auto p = invIt->second.find("Present");
        if (p != invIt->second.end())
        {
            const bool* val = std::get_if<bool>(&p->second);
            if (val)
            {
                state.present = *val;
                lg2::info(
                    "ChassisManager: {PATH} Present (iface added) = {VAL}",
                    "PATH", path, "VAL", *val);
                changed = true;
            }
        }
    }

    // Check Availability / Available (only when configured)
    if (state.checkAvailability)
    {
        auto avIt = ifaces.find(AVAILABILITY_INTF);
        if (avIt != ifaces.end())
        {
            auto p = avIt->second.find("Available");
            if (p != avIt->second.end())
            {
                const bool* val = std::get_if<bool>(&p->second);
                if (val)
                {
                    state.available = *val;
                    lg2::info(
                        "ChassisManager: {PATH} Available (iface added) = {VAL}",
                        "PATH", path, "VAL", *val);
                    changed = true;
                }
            }
        }
    }

    if (changed && _onChassisStatusChanged)
    {
        _onChassisStatusChanged(path);
    }
}

void ChassisManager::watchFanSensorAppear(const std::string& chassisPath,
                                          const std::string& sensorPath,
                                          const std::string& targetInterface)
{
    auto& watches = _fanSensorWatches[chassisPath];
    if (watches.contains(sensorPath))
    {
        return;
    }

    lg2::debug("ChassisManager: {SENSOR} not yet on D-Bus, installing "
               "appear-watch to bind it when it shows up (chassis {PATH})",
               "SENSOR", sensorPath, "PATH", chassisPath);

    watches[sensorPath] = std::make_unique<sdbusplus::match>(
        *_bus, sdbusplus::match_rules::interfacesAddedAtPath(sensorPath),
        [this, chassisPath, sensorPath,
         targetInterface](sdbusplus::message_t& msg) {
            try
            {
                auto [objPath, ifaces] = msg.unpack<
                    sdbusplus::object_path,
                    std::map<std::string,
                             std::map<std::string, PropertyVariantType>>>();

                if (!ifaces.contains(targetInterface))
                {
                    return;
                }
            }
            catch (const std::exception& e)
            {
                lg2::error(
                    "ChassisManager: failed to unpack InterfacesAdded for "
                    "{SENSOR}: {ERR}",
                    "SENSOR", sensorPath, "ERR", e.what());
                return;
            }

            lg2::info(
                "ChassisManager: {SENSOR} appeared on D-Bus "
                "(chassis {PATH}), clearing appear-watch and binding fan sensor",
                "SENSOR", sensorPath, "PATH", chassisPath);

            if (_onFanSensorAppeared)
            {
                _onFanSensorAppeared(chassisPath, sensorPath);
            }
        });
}

void ChassisManager::clearFanSensorAppearWatches(const std::string& chassisPath,
                                                 const std::string& sensorPath)
{
    auto it = _fanSensorWatches.find(chassisPath);
    if (it == _fanSensorWatches.end())
    {
        return;
    }
    if (!sensorPath.empty())
    {
        // Clear only this one sensor's appear-watch; others remain pending.
        auto sIt = it->second.find(sensorPath);
        if (sIt != it->second.end())
        {
            lg2::debug("ChassisManager: clearing appear-watch for {SENSOR} "
                       "(chassis {PATH}) - fan sensor is now bound",
                       "SENSOR", sensorPath, "PATH", chassisPath);
            it->second.erase(sIt);
        }
    }
    else
    {
        // Clear all appear-watches for this chassis.
        lg2::debug("ChassisManager: clearing all {N} appear-watch(es) for "
                   "chassis {PATH} - all fan sensors now bound",
                   "N", it->second.size(), "PATH", chassisPath);
        it->second.clear();
    }
}

void ChassisManager::watchFanSensorLoss(const std::string& chassisPath,
                                        const std::string& sensorPath,
                                        const std::string& targetInterface)
{
    auto& watches = _fanSensorLossWatches[chassisPath];
    if (watches.contains(sensorPath))
    {
        return;
    }

    lg2::debug("ChassisManager: {SENSOR} is live, installing disappear-watch "
               "to detect if it drops off D-Bus (chassis {PATH})",
               "SENSOR", sensorPath, "PATH", chassisPath);

    watches[sensorPath] = std::make_unique<sdbusplus::match>(
        *_bus, sdbusplus::match_rules::interfacesRemovedAtPath(sensorPath),
        [this, chassisPath, sensorPath,
         targetInterface](sdbusplus::message_t& msg) {
            try
            {
                auto [objPath, ifaces] = msg.unpack<sdbusplus::object_path,
                                                    std::vector<std::string>>();

                if (!std::ranges::contains(ifaces, targetInterface))
                {
                    return;
                }
            }
            catch (const std::exception& e)
            {
                lg2::error(
                    "ChassisManager: failed to unpack InterfacesRemoved for "
                    "{SENSOR}: {ERR}",
                    "SENSOR", sensorPath, "ERR", e.what());
                return;
            }

            lg2::info("ChassisManager: {SENSOR} disappeared from D-Bus "
                      "(chassis {PATH}), removing fan sensor and installing "
                      "appear-watch for recovery",
                      "SENSOR", sensorPath, "PATH", chassisPath);

            if (_onFanSensorLost)
            {
                _onFanSensorLost(chassisPath, sensorPath, targetInterface);
            }
        });
}

void ChassisManager::clearFanSensorLossWatches(const std::string& chassisPath,
                                               const std::string& sensorPath)
{
    auto it = _fanSensorLossWatches.find(chassisPath);
    if (it == _fanSensorLossWatches.end())
    {
        return;
    }
    if (!sensorPath.empty())
    {
        // Clear only this one sensor's watch; other fan sensors keep theirs.
        auto sIt = it->second.find(sensorPath);
        if (sIt != it->second.end())
        {
            lg2::debug(
                "ChassisManager: clearing disappear-watch for {SENSOR} "
                "(chassis {PATH}) - fan sensor removed, appear-watch installed",
                "SENSOR", sensorPath, "PATH", chassisPath);
            it->second.erase(sIt);
        }
    }
    else
    {
        // Clear all loss watches for this chassis.
        lg2::debug("ChassisManager: clearing all {N} disappear-watch(es) for "
                   "chassis {PATH} - chassis went unavailable",
                   "N", it->second.size(), "PATH", chassisPath);
        it->second.clear();
    }
}

} // namespace phosphor::fan::control::json
