/**
 * Copyright © 2022 IBM Corporation
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

#include "chassis_manager.hpp"
#include "config_base.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>

#include <map>
#include <vector>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

/**
 * @class Fan - Represents a configured fan control fan object
 *
 * A fan object contains the configured attributes for a fan within the system
 * that will be controlled by the fan control application. These configuration
 * attributes include, but are not limited to, the cooling zone in which the
 * fan is included, what sensors make up the fan, the target interface to be
 * used in setting a target, and any profiles(OPTIONAL) the fan should be
 * included in.
 *
 * (When no profile for a fan is given, the fan defaults to always be included)
 *
 */
class Fan : public ConfigBase
{
    friend class Zone;

  public:
    /* JSON file name for fans */
    static constexpr auto confFileName = "fans.json";

    Fan() = delete;
    Fan(const Fan&) = delete;
    Fan(Fan&&) = delete;
    Fan& operator=(const Fan&) = delete;
    Fan& operator=(Fan&&) = delete;
    ~Fan() = default;

    /**
     * Constructor
     * Parses and populates a zone fan from JSON object data (no D-Bus I/O).
     * Call initSensors() afterwards to resolve sensor services on D-Bus.
     *
     * @param[in] jsonObj - JSON object
     * @param[in] cm      - ChassisManager reference for availability gating
     */
    Fan(const json& jsonObj, ChassisManager& cm);

    /**
     * @brief Resolve sensor D-Bus services and read the current target.
     *
     * Must be called after ChassisManager has been fully initialised
     * (registerChassis() has been called for every chassis path).  It is safe
     * to call initSensors() multiple times; subsequent calls are no-ops when
     * _sensors is already populated.
     *
     * Separated from the constructor so that the caller can first parse ALL
     * fans from JSON (to collect chassis paths), then initialise
     * ChassisManager, and finally call initSensors() on each fan - avoiding
     * the need to parse fans.json twice.
     *
     * @param[in] hintPath    - Sensor object path whose owning service is
     *            already known, or empty
     * @param[in] hintService - Bus name serving @p hintPath.  Used only when
     *            the ObjectMapper lookup for that path fails, which happens
     *            routinely when binding straight off an InterfacesAdded
     *            signal: the mapper is a separate daemon and may not have
     *            processed the same signal yet.
     */
    void initSensors(const std::string& hintPath = {},
                     const std::string& hintService = {});

    /**
     * @brief Return the D-Bus object paths of this fan's sensors.
     *
     * Derived from the configured sensor names and optional "target_path"
     * with no D-Bus access at all, so a caller can subscribe to a sensor's
     * InterfacesAdded *before* attempting the service lookup that would
     * otherwise race it.  Looking up first and subscribing second leaves a
     * window in which the signal has already fired, and D-Bus never replays
     * it.
     *
     * These are the paths the fan *would* bind; getSensors() reports the
     * ones actually resolved.
     */
    std::vector<std::string> getSensorPaths() const;

    /**
     * @brief Get the zone
     *
     * @return Zone this fan belongs in
     */
    inline const auto& getZone() const
    {
        return _zone;
    }

    /**
     * @brief Get the list of sensors
     *
     * @return List of sensors with `Target` property
     */
    inline const auto& getSensors() const
    {
        return _sensors;
    }

    /**
     * @brief Get the sensors' interface
     *
     * @return Interface containing `Target` to use on sensors
     */
    inline const auto& getInterface() const
    {
        return _interface;
    }

    /**
     * @brief Get the current fan target
     *
     * @return - The current target of the fan
     */
    inline auto getTarget() const
    {
        return _target;
    }

    /**
     * @brief Returns true if the fan's sensors were found on D-Bus after
     * initSensors() was called.
     *
     * Will be false for fans whose chassis was not ready (not present, or not
     * available when so configured) at initSensors() time, or for fans whose
     * sensor service had not yet appeared on D-Bus.  The Manager skips adding
     * such fans to zones; ChassisManager fires handleChassisStatusChange()
     * when the chassis becomes ready or a sensor service appears.
     */
    inline bool hasSensorsOnDbus() const
    {
        return !_sensors.empty();
    }

    /**
     * @brief Returns the first sensor D-Bus path that could not be resolved
     *        at initSensors() time because the sensor service was absent.
     *
     * Non-empty only when hasSensorsOnDbus() == false and the chassis was
     * ready (i.e. the chassis was present/available but the fan's sensor
     * service had not yet appeared on D-Bus).  Used by
     * Manager::handleChassisStatusChange() to install an InterfacesAdded watch
     * via ChassisManager::watchFanSensor().
     *
     * A single path is sufficient: when the watch fires,
     * handleChassisStatusChange() calls getConfig<Fan>() from scratch, which
     * re-attempts ALL sensor lookups for the fan.  Watching one path is
     * enough to trigger that retry.
     *
     * Returns an empty string for fans whose chassis was not ready at
     * initSensors() time (those never attempted a D-Bus lookup).
     */
    inline const std::string& getPendingSensorPath() const
    {
        return _pendingSensorPath;
    }

    /**
     * @brief Return the chassis inventory path this fan belongs to, if any.
     *
     * Empty string for fans that do not specify a chassis_path (i.e. fans on
     * non-multi-chassis systems where no chassis gating is required).
     */
    inline const std::string& getChassisPath() const
    {
        return _chassisPath;
    }

    /**
     * @brief Returns whether this fan should gate on the chassis Availability
     *        property in addition to the Present property.
     *
     * Reflects the "check_chassis_availability" JSON key.  Used by
     * Manager::initChassisManager() to call registerChassis() with the correct
     * flag without having to re-parse fans.json.
     */
    inline bool getCheckChassisAvailability() const
    {
        return _checkChassisAvailability;
    }

    /**
     * Sets the target value on all contained sensors
     *
     * @param[in] target - The value to set
     */
    void setTarget(uint64_t target);

    /**
     * @brief Returns the fan's locked targets.
     *
     * @return - vector of locked targets
     */
    const std::vector<uint64_t>& getLockedTargets() const
    {
        return _lockedTargets;
    }

  private:
    /**
     * Forces all contained sensors to the target (if this target is the
     * highest. May be overridden by existing or subsequent higher values),
     * ignoring subsequent setTarget() commands
     *
     * @param[in] target - The target lock to set/add
     */
    void lockTarget(uint64_t target);

    /**
     * Removes the provided target lock from the list of locks. Fan will unlock
     * (become eligible for setTarget()) when all locks are removed from the
     * list.
     */
    void unlockTarget(uint64_t target);

    /* ChassisManager reference for availability gating */
    ChassisManager& _cm;

    /* The sdbusplus bus object */
    sdbusplus::bus_t& _bus;

    /**
     * Interface containing the `Target` property
     * to use in controlling the fan's target
     */
    std::string _interface;

    /* Target for this fan */
    uint64_t _target;

    /* list of locked targets active on this fan */
    std::vector<uint64_t> _lockedTargets;

    /**
     * Map of sensors containing the `Target` property on
     * dbus to the service providing them that make up the fan
     */
    std::map<std::string, std::string> _sensors;

    /* The zone this fan belongs to */
    std::string _zone;

    /**
     * @brief Full D-Bus inventory path of the chassis sled this fan belongs
     * to, e.g. /xyz/openbmc_project/inventory/system/chassis1.
     * Empty for fans that do not carry a "chassis_path" key in JSON.
     */
    std::string _chassisPath;

    /**
     * @brief Whether to gate on the chassis Availability property.
     * Parsed from "check_chassis_availability" JSON key by setChassisPath().
     */
    bool _checkChassisAvailability{false};

    /**
     * @brief Sensor names from the "sensors" JSON key, resolved to full D-Bus
     *        paths in setSensors().  Stored to avoid re-parsing fans.json on
     *        each hotplug retry.
     */
    std::vector<std::string> _sensorNames;

    /**
     * @brief Optional prefix from the "target_path" JSON key.  Empty string
     *        means use the default FAN_SENSOR_PATH prefix.
     */
    std::string _targetPath;

    /**
     * @brief The first sensor D-Bus path whose service could not be resolved
     *        at initSensors() time.  Set by setSensors() on the first failed
     *        lookup; empty string otherwise.
     *
     * One path is sufficient to trigger a hotplug retry via
     * ChassisManager::watchFanSensor() - the retry reconstructs all sensors
     * from scratch so watching a single path is enough.
     */
    std::string _pendingSensorPath;

    /**
     * @brief Parse and set the fan's sensor interface
     *
     * @param[in] jsonObj - JSON object for the fan
     *
     * Sets the sensor interface to use when setting the `Target` property
     */
    void setInterface(const json& jsonObj);

    /**
     * @brief Resolve sensor D-Bus paths from the stored sensor name list.
     *
     * Called by initSensors() after ChassisManager is ready.  Checks
     * cm.isReady(_chassisPath) before attempting D-Bus service lookups.
     */
    /**
     * @brief Build the D-Bus object path for one configured sensor name.
     *
     * @param[in] sensorName - Sensor name from the "sensors" array
     */
    std::string sensorPath(const std::string& sensorName) const;

    void setSensors(const std::string& hintPath = {},
                    const std::string& hintService = {});

    /**
     * @brief Parse and set the fan's zone
     *
     * @param[in] jsonObj - JSON object for the fan
     *
     * Sets the zone this fan is included in.
     */
    void setZone(const json& jsonObj);

    /**
     * @brief Parse and set the fan's chassis path and availability flag
     * (OPTIONAL)
     *
     * @param[in] jsonObj - JSON object for the fan
     *
     * Reads the optional "chassis_path" and "check_chassis_availability" keys.
     * When present the fan defers sensor binding until the chassis is ready
     * according to ChassisManager.
     *
     * "chassis_path" being absent is a valid, normal state
     * for any fan on a non-multi-chassis system.
     */
    void setChassisPath(const json& jsonObj);
};

} // namespace phosphor::fan::control::json
