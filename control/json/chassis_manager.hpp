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

#include "../../utility.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>

#include <functional>
#include <map>
#include <memory>
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
 *  - present == true, AND
 *  - (checkAvailability == false  OR  available == true)
 */
struct ChassisState
{
    /** Full D-Bus inventory path, e.g.
     *  /xyz/openbmc_project/inventory/system/chassis1 */
    std::string path;

    /** Whether to gate on the Availability interface in addition to Present */
    bool checkAvailability{false};

    /** Cached value of xyz.openbmc_project.Inventory.Item / Present */
    bool present{false};

    /** Cached value of Availability / Available (only meaningful when
     *  checkAvailability == true) */
    bool available{false};

    /** True when the Present interface has been seen at least once */
    bool presentIfaceSeen{false};

    /** True when the Availability interface has been seen at least once */
    bool availIfaceSeen{false};

    /**
     * @brief Returns true when the chassis is ready to have its fans
     *        controlled.
     */
    bool isReady() const
    {
        if (checkAvailability)
        {
            return available && availIfaceSeen && present && presentIfaceSeen;
        }
        return present && presentIfaceSeen;
    }
};

/**
 * @class ChassisManager
 *
 * Singleton that tracks the Present and (optionally) Available D-Bus
 * properties for every chassis referenced by the fan control configuration.
 * When any tracked state changes, a caller-supplied callback is invoked so
 * that the Manager can re-evaluate which fans should be active.
 *
 * Usage pattern (inside Manager::load()):
 *
 *   auto& cm = ChassisManager::instance();
 *   cm.init(bus, [this](const std::string& p){ addFansToChassisZones(p); });
 *   cm.registerChassis(path, checkAvailability);
 *   // ... for every unique chassis path seen in fans.json ...
 *
 * Fan::setSensors() calls ChassisManager::instance().isReady(path) to
 * decide whether to bind D-Bus sensor paths for a given chassis.
 */
class ChassisManager
{
  public:
    ChassisManager(const ChassisManager&) = delete;
    ChassisManager(ChassisManager&&) = delete;
    ChassisManager& operator=(const ChassisManager&) = delete;
    ChassisManager& operator=(ChassisManager&&) = delete;
    ~ChassisManager() = default;

    /** @brief Return the single instance. */
    static ChassisManager& instance();

    /**
     * @brief Initialise (or re-initialise) the manager.
     *
     * Clears any previously registered chassis, discards all D-Bus match
     * objects, and stores the bus reference and reload callback.  Must be
     * called before registerChassis().
     *
     * @param[in] bus      - sdbusplus bus reference
     * @param[in] onChassisStatusChanged - Called with the chassis inventory
     *                                     path whenever its ready-state
     *                                     (present/available) may have changed.
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
     * @param[in] path                - Full D-Bus inventory path
     * @param[in] checkAvailability   - Also gate on Availability / Available
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
    ChassisManager() = default;

    /**
     * @brief propertiesChanged callback for the Present property.
     *
     * @param[in] path - The chassis path this subscription belongs to
     * @param[in] msg  - The D-Bus signal message
     */
    void presentPropertyChanged(const std::string& path,
                                sdbusplus::message_t& msg);

    /**
     * @brief propertiesChanged callback for the Available property.
     *
     * @param[in] path - The chassis path this subscription belongs to
     * @param[in] msg  - The D-Bus signal message
     */
    void availPropertyChanged(const std::string& path,
                              sdbusplus::message_t& msg);

    /**
     * @brief interfacesAdded callback for the Present property.
     *
     * @param[in] path - The chassis path this subscription belongs to
     * @param[in] msg  - The D-Bus signal message
     */
    void presentIfaceAdded(const std::string& path, sdbusplus::message_t& msg);

    /**
     * @brief interfacesAdded callback for the Available property.
     *
     * @param[in] path - The chassis path this subscription belongs to
     * @param[in] msg  - The D-Bus signal message
     */
    void availIfaceAdded(const std::string& path, sdbusplus::message_t& msg);

    /** Bus pointer — set by init() */
    sdbusplus::bus_t* _bus{nullptr};

    /** Callback invoked with the chassis path when its ready-state changes */
    std::function<void(const std::string&)> _onChassisStatusChanged;

    /** Tracked chassis states, keyed by full inventory path */
    std::map<std::string, ChassisState> _chassis;

    /** Match objects for propertiesChanged on the Present property */
    std::vector<std::unique_ptr<sdbusplus::match>> _presentPropMatches;

    /** Match objects for propertiesChanged on the Available property */
    std::vector<std::unique_ptr<sdbusplus::match>> _availPropMatches;

    /** Match objects for interfacesAdded carrying the Present property */
    std::vector<std::unique_ptr<sdbusplus::match>> _presentIfaceMatches;

    /** Match objects for interfacesAdded carrying the Available property */
    std::vector<std::unique_ptr<sdbusplus::match>> _availIfaceMatches;
};

} // namespace phosphor::fan::control::json
