/**
 * Copyright © 2020 IBM Corporation
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

#include <any>
#include <functional>
#include <map>
#include <tuple>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

/* Interface property handler function */
using propertyHandler = std::function<std::any(const json&)>;

/**
 * @class Zone - Represents a configured fan control zone
 *
 * A zone object contains the configured attributes for a zone that groups
 * a number of fans together to be under the same speed control. These
 * configuration attributes include, but are not limited to, the full speed
 * of the fans within the zone, a default floor speed, the delay between speed
 * increases, a decrease interval, and any profiles(OPTIONAL) the zone should
 * be included in.
 *
 * (When no profile for a zone is given, the zone defaults to always exist)
 *
 */
class Zone : public ConfigBase
{
  public:
    /* JSON file name for zones */
    static constexpr auto confFileName = "zones.json";

    /* Map of interfaces to properties with their values and persistency */
    static constexpr auto propValue = 0;
    static constexpr auto propPersist = 1;
    using Interfaces =
        std::map<std::string,
                 std::map<std::string, std::tuple<std::any, bool>>>;

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
     * @param[in] bus - sdbusplus bus object
     * @param[in] jsonObj - JSON object
     */
    Zone(sdbusplus::bus::bus& bus, const json& jsonObj);

    /**
     * @brief Get the full speed
     *
     * Full speed is the speed set to the fans within this zone unless there
     * are events configured that alter the fan speeds.
     *
     * @return Full speed of this zone
     */
    inline const auto& getFullSpeed() const
    {
        return _fullSpeed;
    }

    /**
     * @brief Get the default floor speed
     *
     * The default floor speed is the lowest speed the fans within this zone
     * are allowed to decrease to. The zone's floor speed defaults to this
     * unless changed by some configured event.
     *
     * @return Default floor speed
     */
    inline const auto& getDefaultFloor() const
    {
        return _defaultFloor;
    }

    /**
     * @brief Get the speed increase delay(OPTIONAL)
     *
     * The speed increase delay is the amount of time(in seconds) increases
     * to a target speed are delayed before being made. The default is 0, which
     * results in immediate speed increase requests when any events result in
     * a change to the target speed.
     *
     * It is recommend a value other than 0 is configured, but that inherently
     * depends on the fan controller and configured speed increases.
     *
     * @return Speed increase delay(in seconds)
     */
    inline const auto& getIncDelay() const
    {
        return _incDelay;
    }

    /**
     * @brief Get the speed decrease interval
     *
     * Speed decreases happen on a set interval when no requests for an increase
     * in fan speeds exists. This is the interval(in seconds) at which the fans
     * within the zone are decreased if events exist that result in a target
     * speed decrease.
     *
     * @return Speed decrease interval(in seconds)
     */
    inline const auto& getDecInterval() const
    {
        return _decInterval;
    }

    /**
     * @brief Get the configuration of interfaces
     *
     * Interfaces hosted by a zone can optionally be configured to set their
     * property values and/or persistency. These interfaces must be supported
     * by the zone object they are configured for.
     *
     * @return Map of interfaces to properties with their values and persistency
     */
    inline const auto& getInterfaces() const
    {
        return _interfaces;
    }

  private:
    /* The zone's full speed value for fans */
    uint64_t _fullSpeed;

    /* The zone's default floor speed value for fans */
    uint64_t _defaultFloor;

    /* Zone's speed increase delay(in seconds) (OPTIONAL) */
    uint64_t _incDelay;

    /* Zone's speed decrease interval(in seconds) */
    uint64_t _decInterval;

    /* Map of interfaces to properties with their values and persistency */
    Interfaces _interfaces;

    /* Interface properties mapping to their associated handler function */
    static const std::map<std::string, propertyHandler> _props;

    /**
     * @brief Parse and set the zone's full speed value
     *
     * @param[in] jsonObj - JSON object for the zone
     *
     * Sets the full speed value for the zone from the JSON configuration object
     */
    void setFullSpeed(const json& jsonObj);

    /**
     * @brief Parse and set the zone's default floor speed value
     *
     * @param[in] jsonObj - JSON object for the zone
     *
     * Sets the default floor speed value for the zone from the JSON
     * configuration object
     */
    void setDefaultFloor(const json& jsonObj);

    /**
     * @brief Parse and set the zone's decrease interval(in seconds)
     *
     * @param[in] jsonObj - JSON object for the zone
     *
     * Sets the speed decrease interval(in seconds) for the zone from the JSON
     * configuration object
     */
    void setDecInterval(const json& jsonObj);

    /**
     * @brief Parse and set properties on interfaces the zone serves(OPTIONAL)
     *
     * @param[in] jsonObj - JSON object for the zone
     *
     * Sets any properties on the interfaces that the zone to serves along
     * with the property's persistency state (OPTIONAL). A property's "persist"
     * state is defaulted to not be persisted when not given.
     */
    void setInterfaces(const json& jsonObj);
};

/**
 * Properties of interfaces supported by the zone configuration
 */
namespace property
{

/**
 * @brief "Supported" property on the "ThermalMode" interface
 *
 * @param[in] jsonObj - JSON object for the "Supported" property
 *
 * @return - A std::any object of the "Supported" property's value
 */
std::any supported(const json& jsonObj);

/**
 * @brief "Current property on the "ThermalMode" interface
 *
 * @param[in] jsonObj - JSON object for the "Current" property
 *
 * @return - A std::any object of the "Current" property's value
 */
std::any current(const json& jsonObj);

} // namespace property

} // namespace phosphor::fan::control::json
