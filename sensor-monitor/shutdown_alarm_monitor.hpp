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
#include "alarm_checker.hpp"
#include "alarm_handlers.hpp"
#include "alarm_timestamps.hpp"
#include "dbus_alarm_monitor.hpp"
#include "power_state.hpp"
#include "types.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <chrono>
#include <memory>
#include <optional>

namespace sensor::monitor
{

/**
 * @class ShutdownAlarmMonitor
 *
 * This class finds all instances of the D-Bus interfaces
 *   xyz.openbmc_project.Sensor.Threshold.SoftShutdown
 *   xyz.openbmc_project.Sensor.Threshold.HardShutdown
 *
 * If the power state is on at the startup of sensor-monitor
 * the code will check both high and low alarm properties
 * on the interfaces above with AlarmChecker.
 *
 * Then it watches multiple alarm properties by
 * DbusAlarmMonitor, handle the alarm changes by AlarmChecker
 * to perform system protection or recovery actions.
 */
class ShutdownAlarmMonitor
{
  public:
    ShutdownAlarmMonitor() = delete;
    ~ShutdownAlarmMonitor() = default;
    ShutdownAlarmMonitor(const ShutdownAlarmMonitor&) = delete;
    ShutdownAlarmMonitor& operator=(const ShutdownAlarmMonitor&) = delete;
    ShutdownAlarmMonitor(ShutdownAlarmMonitor&&) = delete;
    ShutdownAlarmMonitor& operator=(ShutdownAlarmMonitor&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] bus - The sdbusplus bus object
     * @param[in] event - The sdeventplus event object
     * @param[in] powerState - The PowerState object
     */
    ShutdownAlarmMonitor(sdbusplus::bus_t& bus, sdeventplus::Event& event,
                         std::shared_ptr<phosphor::fan::PowerState> powerState);

  private:
    /**
     * @brief Finds all alarm interfaces currently on
     *        D-Bus and adds them to the alarms map.
     *
     * @param[in] types - The type of alarms to find.
     */
    void findAlarms(std::vector<AlarmType> types);

    /**
     * @brief The power state changed handler.
     *
     * Checks alarms when power is turned on, and clears
     * any running timers on a power off.
     *
     * @param[in] powerStateOn - If the power is now on or off.
     */
    void powerStateChanged(bool powerStateOn);

    /**
     * @brief The sdbusplus bus object
     */
    sdbusplus::bus_t& _bus;

    /**
     * @brief The PowerState object to track power state changes.
     */
    std::shared_ptr<phosphor::fan::PowerState> _powerState;

    /**
     * @brief The map of alarms.
     */
    std::map<AlarmKey, std::unique_ptr<sdeventplus::utility::Timer<
                           sdeventplus::ClockId::Monotonic>>>
        _alarms;

    /**
     * @brief The AlarmChecker
     */
    AlarmChecker _alarmChecker;

    /**
     * @brief The DbusAlarmMonitor
     */
    DbusAlarmMonitor _dbusAlarmMonitor;
};

} // namespace sensor::monitor
