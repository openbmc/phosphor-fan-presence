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
    std::function<void(const std::string&)> onChassisStatusChanged)
{
    _bus = &bus;
    _onChassisStatusChanged = std::move(onChassisStatusChanged);
    _chassis.clear();
    _presentPropMatches.clear();
    _availPropMatches.clear();
    _presentIfaceMatches.clear();
    _availIfaceMatches.clear();
    _fanSensorWatches.clear();
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

    if (!inserted)
    {
        return;
    }

    // Subscribe to propertiesChanged for Inventory.Item (Present)
    _presentPropMatches.emplace_back(std::make_unique<sdbusplus::match>(
        *_bus,
        sdbusplus::match_rules::propertiesChanged(path, util::INV_ITEM_IFACE),
        [this, path](sdbusplus::message_t& msg) {
            presentPropertyChanged(path, msg);
        }));

    // Subscribe to interfacesAdded for Present
    _presentIfaceMatches.emplace_back(std::make_unique<sdbusplus::match>(
        *_bus, sdbusplus::match_rules::interfacesAddedAtPath(path),
        [this, path](sdbusplus::message_t& msg) {
            presentIfaceAdded(path, msg);
        }));

    if (checkAvailability)
    {
        // Subscribe to propertiesChanged for Availability
        _availPropMatches.emplace_back(std::make_unique<sdbusplus::match>(
            *_bus,
            sdbusplus::match_rules::propertiesChanged(path, AVAILABILITY_INTF),
            [this, path](sdbusplus::message_t& msg) {
                availPropertyChanged(path, msg);
            }));

        // Subscribe to interfacesAdded for Available
        _availIfaceMatches.emplace_back(std::make_unique<sdbusplus::match>(
            *_bus, sdbusplus::match_rules::interfacesAddedAtPath(path),
            [this, path](sdbusplus::message_t& msg) {
                availIfaceAdded(path, msg);
            }));
    }
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

    auto p = props.find("Present");
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

    auto p = props.find("Available");
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

void ChassisManager::presentIfaceAdded(const std::string& path,
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
    auto [objPath, ifaces] =
        msg.unpack<sdbusplus::object_path,
                   std::map<std::string, std::map<std::string, ItemVariant>>>();

    auto invIt = ifaces.find(util::INV_ITEM_IFACE);
    if (invIt == ifaces.end())
    {
        return;
    }

    auto p = invIt->second.find("Present");
    if (p == invIt->second.end())
    {
        return;
    }

    const bool* val = std::get_if<bool>(&p->second);
    if (!val)
    {
        return;
    }

    state.present = *val;
    lg2::info("ChassisManager: {PATH} Present (iface added) = {VAL}", "PATH",
              path, "VAL", *val);
    if (_onChassisStatusChanged)
    {
        _onChassisStatusChanged(path);
    }
}

void ChassisManager::availIfaceAdded(const std::string& path,
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
    auto [objPath, ifaces] = msg.unpack<
        sdbusplus::object_path,
        std::map<std::string, std::map<std::string, AvailVariant>>>();

    auto avIt = ifaces.find(AVAILABILITY_INTF);
    if (avIt == ifaces.end())
    {
        return;
    }

    auto p = avIt->second.find("Available");
    if (p == avIt->second.end())
    {
        return;
    }

    const bool* val = std::get_if<bool>(&p->second);
    if (!val)
    {
        return;
    }

    state.available = *val;
    lg2::info("ChassisManager: {PATH} Available (iface added) = {VAL}", "PATH",
              path, "VAL", *val);
    if (_onChassisStatusChanged)
    {
        _onChassisStatusChanged(path);
    }
}

void ChassisManager::watchFanSensor(const std::string& chassisPath,
                                    const std::string& sensorPath,
                                    const std::string& targetInterface)
{
    auto& watches = _fanSensorWatches[chassisPath];
    if (watches.contains(sensorPath))
    {
        return;
    }

    lg2::debug("ChassisManager: watching for fan sensor {SENSOR} "
               "interface {INTF} (chassis {PATH})",
               "SENSOR", sensorPath, "INTF", targetInterface, "PATH",
               chassisPath);

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

            lg2::info("ChassisManager: fan sensor {SENSOR} appeared on D-Bus "
                      "(chassis {PATH}), triggering rebind",
                      "SENSOR", sensorPath, "PATH", chassisPath);

            _fanSensorWatches[chassisPath].erase(sensorPath);

            if (_onChassisStatusChanged)
            {
                _onChassisStatusChanged(chassisPath);
            }
        });
}

void ChassisManager::clearFanSensorWatches(const std::string& chassisPath)
{
    auto it = _fanSensorWatches.find(chassisPath);
    if (it != _fanSensorWatches.end())
    {
        lg2::debug(
            "ChassisManager: clearing {N} fan sensor watch(es) for {PATH}", "N",
            it->second.size(), "PATH", chassisPath);
        it->second.clear();
    }
}

} // namespace phosphor::fan::control::json
