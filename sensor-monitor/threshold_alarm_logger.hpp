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

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdeventplus/event.hpp>

namespace sensor::monitor
{

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
     */
    ThresholdAlarmLogger(sdbusplus::bus::bus& bus, sdeventplus::Event& event);

  private:
    /**
     * @brief The propertiesChanged handler for all of the thresholds
     *        interfaces.
     *
     * Creates event logs for high/low alarm sets and clears.
     *
     * @param[in] msg - The signal message payload.
     */
    void propertiesChanged(sdbusplus::message::message& msg);

    /**
     * @brief The sdbusplus bus object
     */
    sdbusplus::bus::bus& bus;

    /**
     * @brief The sdeventplus Event object
     */
    sdeventplus::Event& event;

    /**
     * @brief The Warning interface match object
     */
    sdbusplus::bus::match::match warningMatch;

    /**
     * @brief The Critical interface match object
     */
    sdbusplus::bus::match::match criticalMatch;

    /**
     * @brief The PerformanceLoss interface match object
     */
    sdbusplus::bus::match::match perfLossMatch;
};

} // namespace sensor::monitor
