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
 * @brief Per-chassis presence/availability state.
 *
 * Ready when Present is true; if checkAvailability is set, Available must
 * also be true.  nullopt means the property has not been seen yet.
 */
struct ChassisState
{
    /** Whether to gate on Available in addition to Present. */
    bool checkAvailability{false};

    /** Inventory.Item / Present - nullopt until first seen. */
    std::optional<bool> present;

    /** Availability / Available - nullopt until first seen;
     *  only evaluated when checkAvailability == true. */
    std::optional<bool> available;

    /** @brief True when the chassis is ready to have its fans controlled. */
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
 * Watches Present (and optionally Available) for every chassis in the fan
 * config.  Calls back into Manager when state changes so it can bind or
 * unbind fans accordingly.
 *
 * Owned by Manager.  Typical setup inside Manager::load():
 *
 *   _chassisMgr->init(bus, event,
 *       std::bind_front(&Manager::handleChassisStatusChange, this),
 *       std::bind_front(&Manager::handleFanSensorAppeared, this),
 *       std::bind_front(&Manager::handleFanSensorLost, this));
 *   _chassisMgr->registerChassis(path, checkAvailability);
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
     * Clears all previously registered chassis and D-Bus matches, then
     * stores the bus, event loop, and callbacks.  Must be called before
     * registerChassis().
     *
     * @param[in] bus   - sdbusplus bus reference
     * @param[in] event - sdeventplus event loop (for deferred work)
     * @param[in] onChassisStatusChanged - Called with the chassis path
     *            whenever its ready-state changes.
     * @param[in] onFanSensorAppeared - Called with (chassisPath, sensorPath,
     *            service) when a fan sensor's target interface appears.
     *            @c service is the InterfacesAdded sender - used to bind
     *            the fan without an ObjectMapper lookup that would race it.
     * @param[in] onFanSensorLost - Called with (chassisPath, sensorPath,
     *            targetInterface) when a bound sensor disappears or its
     *            service exits.
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
     * Reads the current Present/Available state and installs
     * PropertiesChanged and InterfacesAdded matches.  Duplicate calls for
     * the same path are no-ops.
     *
     * @param[in] path              - Full D-Bus inventory path
     * @param[in] checkAvailability - Also gate on Availability/Available
     */
    void registerChassis(const std::string& path, bool checkAvailability);

    /**
     * @brief Returns true when the chassis at @p path is ready for control.
     *
     * An empty @p path (fans with no chassis_path in the config) always
     * returns true, bypassing chassis gating entirely.
     *
     * @param[in] path - Full D-Bus inventory path, or empty string
     */
    bool isReady(const std::string& path) const;

    /** @brief Returns true if no chassis have been registered. */
    bool empty() const
    {
        return _chassis.empty();
    }

    /** @brief Returns the number of registered chassis. */
    std::size_t size() const
    {
        return _chassis.size();
    }

    /**
     * @brief Watch a fan sensor path for its target interface to appear.
     *
     * Calls onFanSensorAppeared when the InterfacesAdded signal arrives.
     * Duplicate calls for the same sensorPath are silently ignored.
     *
     * @param[in] chassisPath      - Chassis inventory path owning this fan
     * @param[in] sensorPath       - Full D-Bus sensor object path
     * @param[in] targetInterface  - Interface to watch for (e.g.
     *            xyz.openbmc_project.Control.FanSpeed)
     */
    void watchFanSensorAppear(const std::string& chassisPath,
                              const std::string& sensorPath,
                              const std::string& targetInterface);

    /**
     * @brief Discard fan-sensor appear-watches for the given chassis.
     *
     * If @p sensorPath is non-empty, only that sensor's watch is removed;
     * otherwise all appear-watches for the chassis are cleared.
     *
     * @param[in] chassisPath - Chassis inventory path
     * @param[in] sensorPath  - Specific sensor to clear, or "" to clear all
     */
    void clearFanSensorAppearWatches(const std::string& chassisPath,
                                     const std::string& sensorPath = "");

    /**
     * @brief Watch a bound fan sensor for disappearance via InterfacesRemoved.
     *
     * Calls onFanSensorLost when the target interface is removed, so the
     * fan can be unbound and a recovery appear-watch installed.
     *
     * @param[in] chassisPath     - Chassis inventory path owning this fan
     * @param[in] sensorPath      - Full D-Bus sensor object path
     * @param[in] targetInterface - Interface to watch for removal
     */
    void watchFanSensorLoss(const std::string& chassisPath,
                            const std::string& sensorPath,
                            const std::string& targetInterface);

    /**
     * @brief Discard fan-sensor loss watches for the given chassis.
     *
     * If @p sensorPath is non-empty, only that sensor's watch is removed;
     * otherwise all loss watches for the chassis are cleared.
     *
     * @param[in] chassisPath - Chassis inventory path
     * @param[in] sensorPath  - Specific sensor to clear, or "" to clear all
     */
    void clearFanSensorLossWatches(const std::string& chassisPath,
                                   const std::string& sensorPath = "");

    /**
     * @brief Watch the owning service of a bound fan sensor for it exiting.
     *
     * InterfacesRemoved is not emitted when a service exits outright; only
     * NameOwnerChanged is.  This watch covers that gap and calls
     * onFanSensorLost when the service drops off the bus.  Installed
     * alongside watchFanSensorLoss() for every bound sensor.
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
     * Same selection semantics as clearFanSensorLossWatches().
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
    /** @brief Called with (chassisPath, sensorPath, service) when a fan
     *         sensor appears.  Bound to Manager::handleFanSensorAppeared.
     *         service is the InterfacesAdded sender - avoids an ObjectMapper
     *         lookup that would race the mapper's own processing. */
    std::function<void(const std::string&, const std::string&,
                       const std::string&)>
        _onFanSensorAppeared;

    /** @brief Called with (chassisPath, sensorPath, targetInterface) when a
     *         bound sensor disappears.  Bound to Manager::handleFanSensorLost.
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

    /** @brief InterfacesAdded handler installed once per chassis; covers
     *         both Inventory.Item/Present and Availability/Available. */
    void ifaceAdded(const std::string& path, sdbusplus::message_t& msg);

    /** @brief Bus pointer - set by init(). */
    sdbusplus::bus_t* _bus{nullptr};

    /** @brief Event loop pointer - set by init(), used by deferWork(). */
    const sdeventplus::Event* _event{nullptr};

    /** @brief Called when a chassis ready-state changes.
     *         Bound to Manager::handleChassisStatusChange. */
    std::function<void(const std::string&)> _onChassisStatusChanged;

    /** @brief Chassis states keyed by inventory path. */
    std::map<std::string, ChassisState> _chassis;

    /** @brief PropertiesChanged matches for Inventory.Item/Present.
     *         One per registered chassis. */
    std::vector<std::unique_ptr<sdbusplus::match>> _presentPropMatches;

    /** @brief PropertiesChanged matches for Availability/Available.
     *         One per chassis with checkAvailability == true. */
    std::vector<std::unique_ptr<sdbusplus::match>> _availPropMatches;

    /** @brief InterfacesAdded matches, one per registered chassis.
     *         Each fires ifaceAdded() for both Present and Available. */
    std::vector<std::unique_ptr<sdbusplus::match>> _ifaceAddedMatches;

    /** @brief Appear-watches: chassisPath -> sensorPath -> match.
     *         Fires _onFanSensorAppeared when the sensor's target interface
     *         shows up on D-Bus. */
    std::map<std::string,
             std::map<std::string, std::unique_ptr<sdbusplus::match>>>
        _fanSensorWatches;

    /** @brief Loss watches: chassisPath -> sensorPath -> match.
     *         Fires _onFanSensorLost on InterfacesRemoved. */
    std::map<std::string,
             std::map<std::string, std::unique_ptr<sdbusplus::match>>>
        _fanSensorLossWatches;

    /** @brief Owner watches: chassisPath -> sensorPath -> match.
     *         Fires _onFanSensorLost on NameOwnerChanged (service exit).
     *         Covers the case InterfacesRemoved does not report. */
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
