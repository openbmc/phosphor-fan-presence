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
#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace phosphor::fan::control::json
{

// D-Bus interface for the Availability / Available property (optional gating)
constexpr auto AVAILABILITY_INTF =
    "xyz.openbmc_project.State.Decorator.Availability";

/**
 * @brief Tracks the presence/availability state of a chassis within a
 *        multi-chassis system.
 *
 * For each chassis registered with ChassisManager:
 *  - Present (xyz.openbmc_project.Inventory.Item / Present) is checked by
 *    default.
 *  - Available (xyz.openbmc_project.State.Decorator.Availability / Available)
 *    is only checked when checkAvailability is true (configured via JSON
 *    "check_chassis_availability": true on the fan entry).
 *
 * A chassis is considered "ready for control" when:
 *  - present has a value and is true, AND
 *  - (checkAvailability == false OR available has a value and is true)
 *
 * std::nullopt means the D-Bus interface has not yet been seen; this avoids
 * the need for a separate "interface seen" flag.
 */
struct ChassisState
{
    /** Full D-Bus inventory path, e.g.
     *  /xyz/openbmc_project/inventory/system/chassis1 */
    std::string path;

    /** Whether to gate on the Availability interface in addition to Present */
    bool checkAvailability{false};

    /** Inventory.Item / Present - nullopt until the interface is first seen */
    std::optional<bool> present;

    /** Availability / Available - nullopt until the interface is first seen.
     *  Only evaluated when checkAvailability == true. */
    std::optional<bool> available;

    /**
     * @brief Returns true when the chassis is ready to have its fans
     *        controlled.
     */
    bool isReady() const
    {
        if (checkAvailability)
        {
            return present.value_or(false) && available.value_or(false);
        }
        return present.value_or(false);
    }
};

/**
 * @class ChassisManager
 *
 * Tracks the Present and (optionally) Available D-Bus properties for every
 * chassis referenced by the fan control configuration.  When any tracked
 * state changes, a caller-supplied callback is invoked so that the Manager
 * can re-evaluate which fans should be active.
 *
 * Owned by Manager via unique_ptr.  Usage pattern (inside Manager::load()):
 *
 *   _chassisMgr = std::make_unique<ChassisManager>();
 *   _chassisMgr->init(bus, [this](const auto& p){ addFansToChassisZones(p); });
 *   _chassisMgr->registerChassis(path, checkAvailability);
 *
 * Fan::setSensors() calls cm.isReady(path) to decide whether to bind
 * D-Bus sensor paths for a given chassis.
 */
class ChassisManager
{
  public:
    ChassisManager() = default;
    ChassisManager(const ChassisManager&) = delete;
    ChassisManager(ChassisManager&&) = delete;
    ChassisManager& operator=(const ChassisManager&) = delete;
    ChassisManager& operator=(ChassisManager&&) = delete;
    ~ChassisManager() = default;

    /**
     * @brief Initialise (or re-initialise) the manager.
     *
     * Clears any previously registered chassis, discards all D-Bus match
     * objects, and stores the bus reference and reload callback.  Must be
     * called before registerChassis().
     *
     * @param[in] bus - sdbusplus bus reference
     * @param[in] onChassisStatusChanged - Called with the chassis inventory
     *            path whenever its ready-state (present/available) changes.
     */
    void init(sdbusplus::bus_t& bus,
              std::function<void(const std::string&)> onChassisStatusChanged);

    /**
     * @brief Register a chassis inventory path to be tracked.
     *
     * Reads the current Present (and optionally Available) value from D-Bus
     * and sets up propertiesChanged / interfacesAdded match subscriptions.
     *
     * Safe to call multiple times with the same path; subsequent calls for
     * an already-registered path are ignored (no duplicate subscriptions).
     *
     * @param[in] path              - Full D-Bus inventory path
     * @param[in] checkAvailability - Also gate on Availability / Available
     */
    void registerChassis(const std::string& path, bool checkAvailability);

    /**
     * @brief Returns true when the chassis at @p path is ready for fan
     *        control (present, and available if so configured).
     *
     * Returns true for any path that has never been registered (non-multi-
     * chassis fans that carry no chassis_path).
     *
     * @param[in] path - Full D-Bus inventory path, or empty string
     */
    bool isReady(const std::string& path) const;

  private:
    void presentPropertyChanged(const std::string& path,
                                sdbusplus::message_t& msg);

    void availPropertyChanged(const std::string& path,
                              sdbusplus::message_t& msg);

    void presentIfaceAdded(const std::string& path, sdbusplus::message_t& msg);

    void availIfaceAdded(const std::string& path, sdbusplus::message_t& msg);

    /* Bus pointer - set by init() */
    sdbusplus::bus_t* _bus{nullptr};

    /* Callback invoked with the chassis path when its ready-state changes */
    std::function<void(const std::string&)> _onChassisStatusChanged;

    /* Chassis states keyed by inventory path */
    std::map<std::string, ChassisState> _chassis;

    std::vector<std::unique_ptr<sdbusplus::match>> _presentPropMatches;
    std::vector<std::unique_ptr<sdbusplus::match>> _availPropMatches;
    std::vector<std::unique_ptr<sdbusplus::match>> _presentIfaceMatches;
    std::vector<std::unique_ptr<sdbusplus::match>> _availIfaceMatches;
};

} // namespace phosphor::fan::control::json
