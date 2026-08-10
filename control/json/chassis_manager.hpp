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
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/event.hpp>

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
 *    is only checked when checkAvailability is true (set via JSON key
 *    "check_chassis_availability": true on the fan entry).
 *
 * A chassis is ready for control when: if checkAvailability is true, both
 * Present and Available must be true; if checkAvailability is false, only
 * Present need be true.
 *
 * std::nullopt means the D-Bus interface has not yet been seen; this avoids
 * the need for a separate "interface seen" flag.
 */
struct ChassisState
{
    /** Whether to gate on the Availability interface in addition to Present */
    bool checkAvailability{false};

    /** Inventory.Item / Present - nullopt until the interface is first seen */
    std::optional<bool> present;

    /** Availability / Available - nullopt until the interface is first seen.
     *  Only evaluated when checkAvailability == true. */
    std::optional<bool> available;

    /**
     * @brief Returns true when the chassis is ready to have its fans
     *        controlled (i.e. it is present and, when checkAvailability is
     *        set, also flagged as available).  Returns false when either
     *        property has not yet been seen (nullopt) or is false.
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
 *   _chassisMgr->init(bus, event,
 *       std::bind_front(&Manager::handleChassisStatusChange, this),
 *       std::bind_front(&Manager::handleFanSensorAppeared, this),
 *       std::bind_front(&Manager::handleFanSensorLost, this));
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
     * objects, and stores the bus reference and callbacks.
     * Must be called before registerChassis().
     *
     * @param[in] bus   - sdbusplus bus reference
     * @param[in] event - sdeventplus event loop (for deferred work)
     * @param[in] onChassisStatusChanged - Called with the chassis inventory
     *            path whenever its ready-state (present/available) changes
     *            in either direction.
     * @param[in] onFanSensorAppeared - Called with (chassisPath, sensorPath,
     *            service) when a previously-absent fan sensor's target
     *            interface appears on D-Bus.  @c service is the sender of
     *            the InterfacesAdded signal - authoritative and avoids an
     *            ObjectMapper lookup that would race the mapper's own
     *            processing of the same signal.
     * @param[in] onFanSensorLost - Called with (chassisPath, sensorPath,
     *            targetInterface) when a bound fan sensor's target interface
     *            disappears from D-Bus mid-run, or when the service that
     *            owned it drops off the bus.
     */
    void init(sdbusplus::bus_t& bus, const sdeventplus::Event& event,
              std::function<void(const std::string&)> onChassisStatusChanged,
              std::function<void(const std::string&, const std::string&,
                                 const std::string&)>
                  onFanSensorAppeared = nullptr,
              std::function<void(const std::string&, const std::string&,
                                 const std::string&)>
                  onFanSensorLost = nullptr);

    /**
     * @brief Register a chassis inventory path to be tracked.
     *
     * On the first call for a given path: reads the current Present (and
     * optionally Available) value from D-Bus and installs propertiesChanged /
     * interfacesAdded match subscriptions.
     *
     * Subsequent calls with the same path are no-ops: the early-return guard
     * fires before any D-Bus I/O or new subscriptions are created.
     *
     * @param[in] path              - Full D-Bus inventory path
     * @param[in] checkAvailability - Also gate on Availability / Available
     */
    void registerChassis(const std::string& path, bool checkAvailability);

    /**
     * @brief Returns true when the chassis at @p path is present and
     *        available (if availability checking is configured).
     *
     * On systems where fans have no "chassis_path" in the JSON config,
     * @p path will be an empty string and no chassis will have been
     * registered.  In that case this function always returns true,
     * meaning the chassis gate is skipped and fan control proceeds
     * directly to the sensor service lookup.
     *
     * @param[in] path - Full D-Bus inventory path, or empty string
     */
    bool isReady(const std::string& path) const;

    /**
     * @brief Returns true if no chassis have been registered.
     *
     * When no fans carry a "chassis_path" in the JSON config, no chassis
     * are registered and fan control runs without any chassis gating.
     */
    bool empty() const
    {
        return _chassis.empty();
    }

    /**
     * @brief Returns the number of registered chassis.
     */
    std::size_t size() const
    {
        return _chassis.size();
    }

    /**
     * @brief Watch a fan sensor path for its target interface to appear on
     *        D-Bus and call onFanSensorAppeared when it does.
     *
     * Installed for every fan sensor whose service was not yet on D-Bus when
     * its chassis became ready (hasSensorsOnDbus() == false).  When the
     * InterfacesAdded signal arrives, onFanSensorAppeared is called so the
     * Manager can bind the fan to its zone.
     *
     * Safe to call multiple times with the same sensorPath; subsequent calls
     * for an already-watched sensor are silently ignored.
     *
     * @param[in] chassisPath      - Chassis inventory path owning this fan
     * @param[in] sensorPath       - Full D-Bus sensor object path
     * @param[in] targetInterface  - Interface that must appear (e.g.
     *            xyz.openbmc_project.Control.FanSpeed)
     */
    void watchFanSensorAppear(const std::string& chassisPath,
                              const std::string& sensorPath,
                              const std::string& targetInterface);

    /**
     * @brief Discard pending fan-sensor appear-watches for the given chassis.
     *
     * When @p sensorPath is provided, only that sensor's watch is removed and
     * all other pending appear-watches on the same chassis remain intact (used
     * when a single sensor's service appeared and its fan was bound).  When
     * @p sensorPath is omitted (or empty), every appear-watch for the chassis
     * is cleared at once (used when all fans on the chassis are now bound).
     *
     * @param[in] chassisPath - Chassis inventory path
     * @param[in] sensorPath  - Specific sensor to clear, or "" to clear all
     */
    void clearFanSensorAppearWatches(const std::string& chassisPath,
                                     const std::string& sensorPath = "");

    /**
     * @brief Watch a bound fan sensor for disappearance from D-Bus mid-run.
     *
     * Installs an InterfacesRemoved watch at the sensor object path.
     * When the target interface is removed, onFanSensorLost is called so
     * the fan is removed from its zone and an InterfacesAdded recovery
     * watch is installed.
     *
     * @param[in] chassisPath     - Chassis inventory path owning this fan
     * @param[in] sensorPath      - Full D-Bus sensor object path
     * @param[in] targetInterface - Interface to watch for removal
     */
    void watchFanSensorLoss(const std::string& chassisPath,
                            const std::string& sensorPath,
                            const std::string& targetInterface);

    /**
     * @brief Discard fan-sensor-loss watches for the given chassis.
     *
     * When @p sensorPath is provided, only that sensor's watch is removed and
     * all other fans on the chassis keep their watches intact (used when a
     * single sensor disappears mid-run).  When @p sensorPath is omitted (or
     * empty), every loss watch for the chassis is cleared at once (used when
     * the whole chassis goes unavailable).
     *
     * @param[in] chassisPath - Chassis inventory path
     * @param[in] sensorPath  - Specific sensor to clear, or "" to clear all
     */
    void clearFanSensorLossWatches(const std::string& chassisPath,
                                   const std::string& sensorPath = "");

    /**
     * @brief Watch the service that owns a bound fan sensor for it dropping
     *        off the bus.
     *
     * InterfacesRemoved is only emitted when a *running* service explicitly
     * removes an object from its ObjectManager.  When the owning service
     * exits instead - which is what happens when a per-device sensor service
     * is stopped after its hardware is unbound - no InterfacesRemoved is sent
     * and the objects simply vanish with the bus name.  The only signal in
     * that case is NameOwnerChanged with an empty new owner, so a loss watch
     * alone silently misses the removal.
     *
     * Installed alongside watchFanSensorLoss() for every bound fan sensor.
     * Fires the onFanSensorLost callback supplied to init() when @p service
     * loses its owner.
     *
     * @param[in] chassisPath     - Chassis inventory path owning this fan
     * @param[in] sensorPath      - Full D-Bus sensor object path
     * @param[in] service         - Bus name currently serving @p sensorPath
     * @param[in] targetInterface - Interface the fan is bound through
     */
    void watchFanSensorOwner(const std::string& chassisPath,
                             const std::string& sensorPath,
                             const std::string& service,
                             const std::string& targetInterface);

    /**
     * @brief Discard fan-sensor owner watches for the given chassis.
     *
     * Same selection semantics as clearFanSensorLossWatches(): a non-empty
     * @p sensorPath clears just that sensor's watch, an empty one clears
     * every owner watch on the chassis.
     *
     * @param[in] chassisPath - Chassis inventory path
     * @param[in] sensorPath  - Specific sensor to clear, or "" to clear all
     */
    void clearFanSensorOwnerWatches(const std::string& chassisPath,
                                    const std::string& sensorPath = "");

    /**
     * @brief Run @p work on the next event-loop iteration.
     *
     * An sdbusplus::match cannot be erased from inside its own callback
     * (use-after-free).  Route any watch cleanup through here so the erase
     * happens after the dispatch stack unwinds.
     *
     * @param[in] work - Callable to run once, then discard
     */
    void deferWork(std::function<void()> work);

  private:
    /**
     * @brief Called with (chassisPath, sensorPath, service) when a fan
     *        sensor's target interface appears on D-Bus.  Bound to
     *        Manager::handleFanSensorAppeared.
     *
     * @c service is the sender of the InterfacesAdded signal, i.e. the bus
     * name that owns the newly created object.  Passing it through lets the
     * Manager bind the fan without asking the ObjectMapper, which may not
     * have processed the same signal yet.
     */
    std::function<void(const std::string&, const std::string&,
                       const std::string&)>
        _onFanSensorAppeared;

    /**
     * @brief Called with (chassisPath, sensorPath, targetInterface) when a
     *        bound fan sensor's target interface disappears from D-Bus.
     *        Bound to Manager::handleFanSensorLost.
     */
    std::function<void(const std::string&, const std::string&,
                       const std::string&)>
        _onFanSensorLost;

    /** @brief Handle PropertiesChanged for Inventory.Item/Present. */
    void presentPropertyChanged(const std::string& path,
                                sdbusplus::message_t& msg);

    /** @brief Handle PropertiesChanged for Availability/Available. */
    void availPropertyChanged(const std::string& path,
                              sdbusplus::message_t& msg);

    /**
     * @brief Single InterfacesAdded handler installed once per chassis.
     *
     * Handles both Inventory.Item/Present and Availability/Available in
     * one wakeup, avoiding the need for two separate match subscriptions.
     */
    void ifaceAdded(const std::string& path, sdbusplus::message_t& msg);

    /** @brief Bus pointer - set by init() */
    sdbusplus::bus_t* _bus{nullptr};

    /** @brief Event loop pointer - set by init(), used by deferWork() */
    const sdeventplus::Event* _event{nullptr};

    /**
     * @brief Called with the chassis inventory path whenever its
     *        ready-state (present/available) changes in either direction.
     *        Bound to Manager::handleChassisStatusChange.
     */
    std::function<void(const std::string&)> _onChassisStatusChanged;

    /** @brief Chassis states keyed by inventory path. */
    std::map<std::string, ChassisState> _chassis;

    /**
     * @brief PropertiesChanged match objects for Inventory.Item (Present).
     *        One entry per registered chassis; kept alive for the process
     *        lifetime.
     */
    std::vector<std::unique_ptr<sdbusplus::match>> _presentPropMatches;

    /**
     * @brief PropertiesChanged match objects for Availability (Available).
     *        One entry per chassis that has checkAvailability == true.
     */
    std::vector<std::unique_ptr<sdbusplus::match>> _availPropMatches;

    /**
     * @brief InterfacesAdded match objects, one per registered chassis.
     *        Each fires ifaceAdded() and handles both Inventory.Item and
     *        Availability in a single callback.
     */
    std::vector<std::unique_ptr<sdbusplus::match>> _ifaceAddedMatches;

    /**
     * Fan sensor appear-watches: chassisPath -> sensorPath -> match.
     * Each match fires _onFanSensorAppeared when the sensor's target interface
     * appears on D-Bus.
     */
    std::map<std::string,
             std::map<std::string, std::unique_ptr<sdbusplus::match>>>
        _fanSensorWatches;

    /**
     * Fan sensor loss watches: chassisPath -> sensorPath -> match.
     * Each match fires the onFanSensorLost callback when the target interface
     * disappears from D-Bus mid-run.
     */
    std::map<std::string,
             std::map<std::string, std::unique_ptr<sdbusplus::match>>>
        _fanSensorLossWatches;

    /**
     * Fan sensor owner watches: chassisPath -> sensorPath -> match.
     * Each match fires the onFanSensorLost callback when the bus name serving
     * that sensor loses its owner.  Covers the service-exit case that
     * InterfacesRemoved does not report.
     */
    std::map<std::string,
             std::map<std::string, std::unique_ptr<sdbusplus::match>>>
        _fanSensorOwnerWatches;

    /** @brief Deferred watch-cleanup callbacks.
     *
     *  A sdbusplus::match cannot be erased from its own callback
     *  (use-after-free); deferWork() posts the erase here instead.
     *  Spent sources are reaped on the next deferWork() call. */
    std::vector<std::unique_ptr<sdeventplus::source::Defer>> _deferredWork;

    /** @brief Nesting depth of deferred-work callbacks currently running. */
    unsigned _deferDepth{0};
};

} // namespace phosphor::fan::control::json
