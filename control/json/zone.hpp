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
#include "dbus_zone.hpp"
#include "fan.hpp"

#include <nlohmann/json.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <any>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <tuple>

namespace phosphor::fan::control::json
{

class Manager;

using json = nlohmann::json;

/* Dbus event timer */
using Timer = sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>;

/**
 * @class Zone - Represents a configured fan control zone
 *
 * A zone object contains the configured attributes for a zone that groups
 * a number of fans together to be under the same target control. These
 * configuration attributes include, but are not limited to, the default ceiling
 * of the fans within the zone, a default floor, the delay between increases, a
 * decrease interval, and any profiles(OPTIONAL) the zone should be included in.
 *
 * (When no profile for a zone is given, the zone defaults to always exist)
 *
 */
class Zone : public ConfigBase
{
  public:
    /* JSON file name for zones */
    static constexpr auto confFileName = "zones.json";

    Zone() = delete;
    Zone(const Zone&) = delete;
    Zone(Zone&&) = delete;
    Zone& operator=(const Zone&) = delete;
    Zone& operator=(Zone&&) = delete;
    ~Zone() = default;

    /**
     * Constructor
     * Parses and populates a zone from JSON object data
     *
     * @param[in] jsonObj - JSON object
     * @param[in] event - sdeventplus event loop
     * @param[in] mgr - Manager of this zone
     */
    Zone(const json& jsonObj, const sdeventplus::Event& event, Manager* mgr);

    /**
     * @brief Get the poweron target
     *
     * Poweron target is the target the fans within this zone should be set to
     * when the system is powered on.
     *
     * @return Poweron target of this zone
     */
    inline const auto& getPoweronTarget() const
    {
        return _poweronTarget;
    }

    /**
     * @brief Get the default ceiling
     *
     * Default ceiling is the highest target the fans within this zone is
     * allowed to increase to. The zone's ceiling defaults to this unless
     * changed by some configured event.
     *
     * @return Default ceiling of this zone
     */
    inline const auto& getDefaultCeiling() const
    {
        return _defaultCeiling;
    }

    /**
     * @brief Get the default floor
     *
     * The default floor is the lowest target the fans within this zone
     * are allowed to decrease to. The zone's floor defaults to this
     * unless changed by some configured event.
     *
     * @return Default floor
     */
    inline const auto& getDefaultFloor() const
    {
        return _defaultFloor;
    }

    /**
     * @brief Get the increase delay(OPTIONAL)
     *
     * The increase delay is the amount of time(in seconds) increases
     * to a target are delayed before being made. The default is 0, which
     * results in immediate increase requests when any events result in
     * a change to the target.
     *
     * It is recommend a value other than 0 is configured, but that inherently
     * depends on the fan controller and configured increases.
     *
     * @return Increase delay(in seconds)
     */
    inline const auto& getIncDelay() const
    {
        return _incDelay;
    }

    /**
     * @brief Get the decrease interval
     *
     * Decreases happen on a set interval when no requests for an increase
     * in fan targets exists. This is the interval(in seconds) at which the fans
     * within the zone are decreased if events exist that result in a target
     * decrease.
     *
     * @return Decrease interval(in seconds)
     */
    inline const auto& getDecInterval() const
    {
        return _decInterval;
    }

    /**
     * @brief Get the current target of the zone
     *
     * @return - The current target of the zone
     */
    inline const auto& getTarget() const
    {
        return _target;
    }

    /**
     * @brief Get the target increase delta
     *
     * @return - The current target increase delta
     */
    inline auto& getIncDelta() const
    {
        return _incDelta;
    };

    /**
     * @brief Get the target decrease delta
     *
     * @return - The current target decrease delta
     */
    inline auto& getDecDelta() const
    {
        return _decDelta;
    };

    /**
     * @brief Get the manager of the zone
     *
     * @return - The manager of the zone
     */
    inline auto* getManager() const
    {
        return _manager;
    }

    /**
     * @brief Enable the zone
     *
     * Performs the necessary tasks to enable the zone such as restoring any
     * dbus property states(if persisted), starting the decrement timer, etc...
     */
    void enable();

    /**
     * @brief Add a fan object to the zone
     *
     * @param[in] fan - Unique pointer to a fan object that will be moved into
     * the zone
     *
     * Adds a fan object to the list of fans that make up the zone by moving the
     * fan object into the list.
     */
    void addFan(std::unique_ptr<Fan> fan);

    /**
     * Sets all fans in the zone to the target given when the zone is active
     *
     * @param[in] target - Target for fans
     */
    void setTarget(uint64_t target);

    /**
     * Add a target lock for the specified fan.
     *
     * @param[in] fname - Fan to request/add the target lock
     * @param[in] target - Target to register
     */
    void lockFanTarget(const std::string& fname, uint64_t target);

    /**
     * Remove target lock for specific fan.
     *
     * @param[in] fname - Fan to remove lock from
     * @param[in] target- Target to de-register
     */
    void unlockFanTarget(const std::string& fname, uint64_t target);

    /**
     * Sets and holds all fans in the zone to the target given or releases a
     * target hold resulting in the fans being held at the highest remaining
     * hold target if other hold targets had been requested. When no hold
     * targets exist, the zone returns to being active.
     *
     * @param[in] ident - Unique identifier for a target hold
     * @param[in] target - Target to hold fans at
     * @param[in] hold - Whether to hold(true) or release(false) a target hold
     */
    void setTargetHold(const std::string& ident, uint64_t target, bool hold);

    /**
     * @brief Set the floor to the given target and increase target to the floor
     * when the target is below the floor value when floor changes are allowed.
     *
     * @param[in] target - Target to set the floor to
     */
    void setFloor(uint64_t target);

    /**
     * Sets and holds the floor of the zone to the target given or releases a
     * floor hold resulting in the fans being held at the highest remaining
     * hold target if other floor hold targets had been requested. When no hold
     * targets exist, the floor gets set to the default floor value.
     *
     * @param[in] ident - Unique identifier for a floor hold
     * @param[in] target - Floor value
     * @param[in] hold - Whether to hold(true) or release(false) a hold
     */
    void setFloorHold(const std::string& ident, uint64_t target, bool hold);

    /**
     * @brief Says if the passed in identity has a floor hold
     *
     * @param ident - The identity key to check
     * @return bool - If it has a floor hold or not
     */
    inline bool hasFloorHold(const std::string& ident) const
    {
        return _floorHolds.contains(ident);
    }

    /**
     * @brief Set the default floor to the given value
     *
     * @param[in] value - Value to set the default floor to
     */
    inline void setDefaultFloor(uint64_t value)
    {
        _defaultFloor = value;
    }

    /**
     * @brief Sets the floor change allowed state
     *
     * @param[in] ident - An identifier that affects floor changes
     * @param[in] isAllow - Allow state according to the identifier
     */
    inline void setFloorChangeAllow(const std::string& ident, bool isAllow)
    {
        _floorChange[ident] = isAllow;
    }

    /**
     * @brief Sets the decrease allowed state of a group
     *
     * @param[in] ident - An identifier that affects speed decreases
     * @param[in] isAllow - Allow state according to the identifier
     */
    inline void setDecreaseAllow(const std::string& ident, bool isAllow)
    {
        _decAllowed[ident] = isAllow;
    }

    /**
     * @brief Calculate the requested target from the given delta and increases
     * the fans, not going above the ceiling.
     *
     * @param[in] targetDelta - The delta to increase the target by
     */
    void requestIncrease(uint64_t targetDelta);

    /**
     * @brief Callback function for the increase timer that delays
     * processing of requested target increases while fans are increasing
     */
    void incTimerExpired();

    /**
     * @brief Calculate the lowest requested decrease target from the given
     * delta within a decrease interval.
     *
     * @param[in] targetDelta - The delta to decrease the target by
     */
    void requestDecrease(uint64_t targetDelta);

    /**
     * @brief Callback function for the decrease timer that processes any
     * requested target decreases if allowed
     */
    void decTimerExpired();

    /**
     * @brief Set the requested target base to be used as the target to base a
     * new requested target from
     *
     * @param[in] targetBase - Base target value to use
     */
    inline void setRequestTargetBase(uint64_t targetBase)
    {
        _requestTargetBase = targetBase;
    };

    /**
     * @brief Set a property to be persisted
     *
     * @param[in] intf - Interface containing property
     * @param[in] prop - Property to be persisted
     */
    void setPersisted(const std::string& intf, const std::string& prop);

    /**
     * @brief Is the property persisted
     *
     * @param[in] intf - Interface containing property
     * @param[in] prop - Property to check if persisted
     *
     * @return - True if property is to be persisted, false otherwise
     */
    bool isPersisted(const std::string& intf, const std::string& prop) const;

    /**
     * @brief A handler function to set/update a property on a zone
     * @details Sets or updates a zone's dbus property to the given value using
     * the provided base dbus object's set property function
     *
     * @param[in] intf - Interface on zone object
     * @param[in] prop - Property on interface
     * @param[in] func - Zone dbus object's set property function pointer
     * @param[in] value - Value to set property to
     * @param[in] persist - Persist property value or not
     *
     * @return Lambda function
     *     A lambda function to set/update the zone dbus object's property
     */
    template <typename T>
    static auto setProperty(const char* intf, const char* prop,
                            T (DBusZone::*func)(T), T&& value, bool persist)
    {
        return [=, value = std::forward<T>(value)](DBusZone& dbusZone,
                                                   Zone& zone) {
            (dbusZone.*func)(value);
            if (persist)
            {
                zone.setPersisted(intf, prop);
            }
        };
    }

    /**
     * @brief A handler function to set/update a zone's dbus property's persist
     * state
     * @details Sets or updates a zone's dbus property's persist state where the
     * value of the property is to be left unchanged
     *
     * @param[in] intf - Interface on zone object
     * @param[in] prop - Property on interface
     * @param[in] persist - Persist property value or not
     *
     * @return Lambda function
     *     A lambda function to set/update the zone's dbus object's property's
     * persist state
     */
    static auto setPropertyPersist(const char* intf, const char* prop,
                                   bool persist)
    {
        return [=](DBusZone&, Zone& zone) {
            if (persist)
            {
                zone.setPersisted(intf, prop);
            }
        };
    }

    /**
     * @brief Dump the attributes into JSON
     *
     * @return json - JSON object with the attributes
     */
    json dump() const;

  private:
    /* The zone's associated dbus object */
    std::unique_ptr<DBusZone> _dbusZone;

    /* The zone's manager */
    Manager* _manager;

    /* The zone's poweron target value for fans */
    uint64_t _poweronTarget;

    /* The zone's default ceiling value for fans */
    uint64_t _defaultCeiling;

    /* The zone's default floor value for fans */
    uint64_t _defaultFloor;

    /* Zone's increase delay(in seconds) (OPTIONAL) */
    std::chrono::seconds _incDelay;

    /* Zone's decrease interval(in seconds) (OPTIONAL) */
    std::chrono::seconds _decInterval;

    /* The floor target to not go below */
    uint64_t _floor;

    /* Target for this zone */
    uint64_t _target;

    /* Zone increase delta */
    uint64_t _incDelta;

    /* Zone decrease delta */
    uint64_t _decDelta;

    /* The ceiling target to not go above */
    uint64_t _ceiling;

    /* Requested target base */
    uint64_t _requestTargetBase;

    /* Map of whether floor changes are allowed by a string identifier */
    std::map<std::string, bool> _floorChange;

    /* Map of controlling decreases allowed by a string identifer */
    std::map<std::string, bool> _decAllowed;

    /* Map of interfaces to persisted properties the zone hosts*/
    std::map<std::string, std::vector<std::string>> _propsPersisted;

    /* Automatic fan control active state */
    bool _isActive;

    /* The target increase timer object */
    Timer _incTimer;

    /* The target decrease timer object */
    Timer _decTimer;

    /* Map of target holds by a string identifier */
    std::unordered_map<std::string, uint64_t> _targetHolds;

    /* Map of floor holds by a string identifier */
    std::unordered_map<std::string, uint64_t> _floorHolds;

    /* Interface to property mapping of their associated set property handler
     * function */
    static const std::map<
        std::string,
        std::map<std::string, std::function<std::function<void(
                                  DBusZone&, Zone&)>(const json&, bool)>>>
        _intfPropHandlers;

    /* List of fans included in this zone */
    std::vector<std::unique_ptr<Fan>> _fans;

    /* List of configured interface set property functions */
    std::vector<std::function<void(DBusZone&, Zone&)>> _propInitFunctions;

    /**
     * @brief Parse and set the zone's poweron target value
     *
     * @param[in] jsonObj - JSON object for the zone
     *
     * Sets the poweron target value for the zone from the JSON configuration
     * object
     */
    void setPowerOnTarget(const json& jsonObj);

    /**
     * @brief Parse and set the interfaces served by the zone(OPTIONAL)
     *
     * @param[in] jsonObj - JSON object for the zone
     *
     * Constructs any zone interface handler functions for interfaces that the
     * zone serves which contains the interface's property's value and
     * persistency state (OPTIONAL). A property's "persist" state is defaulted
     * to not be persisted when not given.
     */
    void setInterfaces(const json& jsonObj);

    /**
     * @brief Get the request target base if defined, otherwise the the current
     * target is returned
     *
     * @return - The request target base or current target
     */
    inline auto getRequestTargetBase() const
    {
        return (_requestTargetBase != 0) ? _requestTargetBase : _target;
    };
};

/**
 * Properties of interfaces supported by the zone configuration
 */
namespace zone::property
{

/**
 * @brief "Supported" property on the "xyz.openbmc_project.Control.ThermalMode"
 * interface parser. Also creates the handler function for the Zone dbus object
 * to initialize the property according to what's parsed from the configuration.
 *
 * @param[in] jsonObj - JSON object for the "Supported" property
 * @param[in] persist - Whether to persist the value or not
 *
 * @return Zone dbus object's set property function for the "Supported" property
 */
std::function<void(DBusZone&, Zone&)> supported(const json& jsonObj,
                                                bool persist);

/**
 * @brief "Current" property on the "xyz.openbmc_project.Control.ThermalMode"
 * interface parser. Also creates the handler function for the Zone dbus object
 * to initialize the property according to what's parsed from the configuration.
 *
 * @param[in] jsonObj - JSON object for the "Current" property
 * @param[in] persist - Whether to persist the value or not
 *
 * @return Zone dbus object's set property function for the "Current" property
 */
std::function<void(DBusZone&, Zone&)> current(const json& jsonObj,
                                              bool persist);

} // namespace zone::property

} // namespace phosphor::fan::control::json
