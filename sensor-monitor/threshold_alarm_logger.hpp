/**
 * Copyright © 2021 IBM Corporation
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

#include "power_state.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdeventplus/event.hpp>

namespace sensor::monitor
{

using InterfaceName = std::string;
using PropertyName = std::string;
using ErrorName = std::string;
using ObjectPath = std::string;
using InterfaceKey = std::tuple<ObjectPath, InterfaceName>;

/**
 * @class ThresholdAlarmLogger
 *
 * This class watches the threshold interfaces
 *    openbmc_project.Sensor.Threshold.Warning
 *    openbmc_project.Sensor.Threshold.Critical
 *    openbmc_project.Sensor.Threshold.PerformanceLoss
 *
 * and creates event logs when their high and low alarm
 * properties set and clear.  The error names of the
 * event logs are based on the sensor type and look like:
 *
 * xyz.openbmc_project.Sensor.Threshold.Error.TemperatureWarningHigh
 * xyz.openbmc_project.Sensor.Threshold.Error.TemperatureWarningHighClear
 *
 * Event logs are only created when the power is on.
 */
class ThresholdAlarmLogger
{
  public:
    ThresholdAlarmLogger() = delete;
    ~ThresholdAlarmLogger() = default;
    ThresholdAlarmLogger(const ThresholdAlarmLogger&) = default;
    ThresholdAlarmLogger& operator=(const ThresholdAlarmLogger&) = default;
    ThresholdAlarmLogger(ThresholdAlarmLogger&&) = default;
    ThresholdAlarmLogger& operator=(ThresholdAlarmLogger&&) = default;

    /**
     * @brief Constructor
     *
     * Looks for existing active threshold alarms.
     *
     * @param[in] bus - The sdbusplus bus object
     * @param[in] event - The sdeventplus event object
     * @param[in] powerState - The PowerState object
     */
    ThresholdAlarmLogger(sdbusplus::bus_t& bus, sdeventplus::Event& event,
                         std::shared_ptr<phosphor::fan::PowerState> powerState);

  private:
    /**
     * @brief The propertiesChanged handler for all of the thresholds
     *        interfaces.
     *
     * Creates event logs for high/low alarm sets and clears.
     *
     * @param[in] msg - The signal message payload.
     */
    void propertiesChanged(sdbusplus::message_t& msg);

    /**
     * @brief The interfacesRemoved removed handler for the threshold
     *        interfaces.
     *
     * Removes that threshold from the alarms map
     *
     * @param[in] msg - The signal message payload.
     */
    void interfacesRemoved(sdbusplus::message_t& msg);

    /**
     * @brief The interfacesAdded handler for the threshold
     *        interfaces.
     *
     * Checks the alarm when it shows up on D-Bus.
     *
     * @param[in] msg - The signal message payload.
     */
    void interfacesAdded(sdbusplus::message_t& msg);

    /**
     * @brief Checks for alarms in the D-Bus data passed in,
     *        and creates an event log if necessary.
     *
     * @param[in] sensorPath - D-Bus path of the sensor
     * @param[in] interface - The threshold interface name
     * @param[in] properties - The map of property values on the interface
     */
    void checkProperties(
        const std::string& sensorPath, const std::string& interface,
        const std::map<std::string, std::variant<bool>>& properties);

    /**
     * @brief Checks for active alarms on the path and threshold interface
     *        passed in and creates event logs if necessary.
     *
     * @param[in] interface - The threshold interface
     * @param[in] sensorPath - The sensor D-Bus path
     * @param[in] service - The D-Bus service that owns the interface
     */
    void checkThresholds(const std::string& interface,
                         const std::string& sensorPath,
                         const std::string& service);

    /**
     * @brief Checks for all active alarms on all existing
     *        threshold interfaces and creates event logs
     *        if necessary.
     */
    void checkThresholds();

    /**
     * @brief Creates an event log for the alarm set/clear
     *
     * @param[in] sensorPath - The sensor object path
     * @param[in] interface - The threshold interface
     * @param[in] alarmProperty - The alarm property name
     * @param[in] alarmValue - The alarm value
     */
    void createEventLog(const std::string& sensorPath,
                        const std::string& interface,
                        const std::string& alarmProperty, bool alarmValue);

    /**
     * @brief Returns the type of the sensor using the path segment
     *        that precedes the sensor name.
     *
     * /xyz/openbmc_project/sensors/voltage/vout -> type == voltage
     *
     * @param[in] sensorPath - The sensor object path name
     *
     * @return std::string The type segment
     */
    std::string getSensorType(std::string sensorPath);

    /**
     * @brief Allows for skipping event logs based on the sensor type.
     *
     * Specifically for the 'utilization' type because its provider
     * doesn't support configurable thresholds yet.
     *
     * @param[in] type - The sensor type, like 'temperature'.
     * @return bool - If it can be skipped or not.
     */
    bool skipSensorType(const std::string& type);

    /**
     * @brief Returns the inventory path to use for a FRU callout
     *        for the alarm exceeded errors.
     *
     * It finds the path by looking for 'inventory' or 'chassis'
     * association objects on the sensor that point to a FRU.
     *
     * @param[in] std::string - The sensor object path
     * @return std::string - The inventory path for the FRU callout.
     *                       May be empty if none found.
     */
    std::string getCallout(const std::string& sensorPath);

    /**
     * @brief The power state changed handler.
     *
     * Checks alarms when power is turned on.
     *
     * @param[in] powerStateOn - If the power is now on or off.
     */
    void powerStateChanged(bool powerStateOn);

    /**
     * @brief The sdbusplus bus object
     */
    sdbusplus::bus_t& bus;

    /**
     * @brief The sdeventplus Event object
     */
    sdeventplus::Event& event;

    /**
     * @brief The PowerState object to track power state changes.
     */
    std::shared_ptr<phosphor::fan::PowerState> _powerState;

    /**
     * @brief The Warning interface match object
     */
    sdbusplus::bus::match_t warningMatch;

    /**
     * @brief The Critical interface match object
     */
    sdbusplus::bus::match_t criticalMatch;

    /**
     * @brief The PerformanceLoss interface match object
     */
    sdbusplus::bus::match_t perfLossMatch;

    /**
     * @brief The InterfacesRemoved match object
     */
    sdbusplus::bus::match_t ifacesRemovedMatch;

    /**
     * @brief The InterfacesAdded match object
     */
    sdbusplus::bus::match_t ifacesAddedMatch;

    /**
     * @brief The current alarm values
     */
    std::map<InterfaceKey, std::map<PropertyName, bool>> alarms;
};

} // namespace sensor::monitor
