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

#include "config_base.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>

#include <map>

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
     * Parses and populates a zone fan from JSON object data
     *
     * @param[in] jsonObj - JSON object
     */
    explicit Fan(const json& jsonObj);

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
     * @brief Parse and set the fan's sensor interface
     *
     * @param[in] jsonObj - JSON object for the fan
     *
     * Sets the sensor interface to use when setting the `Target` property
     */
    void setInterface(const json& jsonObj);

    /**
     * @brief Parse and set the fan's sensor list
     *
     * @param[in] jsonObj - JSON object for the fan
     *
     * Sets the list of sensors that contain a `Target` property on dbus
     * that make up this fan.
     */
    void setSensors(const json& jsonObj);

    /**
     * @brief Parse and set the fan's zone
     *
     * @param[in] jsonObj - JSON object for the fan
     *
     * Sets the zone this fan is included in.
     */
    void setZone(const json& jsonObj);
};

} // namespace phosphor::fan::control::json
