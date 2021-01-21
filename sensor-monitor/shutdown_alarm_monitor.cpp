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
#include "config.h"

#include "shutdown_alarm_monitor.hpp"

#include <fmt/format.h>

#include <phosphor-logging/log.hpp>

namespace sensor::monitor
{
using namespace phosphor::logging;
using namespace phosphor::fan::util;
using namespace phosphor::fan;
namespace fs = std::filesystem;

const std::map<ShutdownType, std::string> shutdownInterfaces{
    {ShutdownType::hard, "xyz.openbmc_project.Sensor.Threshold.HardShutdown"},
    {ShutdownType::soft, "xyz.openbmc_project.Sensor.Threshold.SoftShutdown"}};

const std::map<ShutdownType, std::map<AlarmType, std::string>> alarmProperties{
    {ShutdownType::hard,
     {{AlarmType::low, "HardShutdownAlarmLow"},
      {AlarmType::high, "HardShutdownAlarmHigh"}}},
    {ShutdownType::soft,
     {{AlarmType::low, "SoftShutdownAlarmLow"},
      {AlarmType::high, "SoftShutdownAlarmHigh"}}}};

using namespace sdbusplus::bus::match;

ShutdownAlarmMonitor::ShutdownAlarmMonitor(sdbusplus::bus::bus& bus,
                                           sdeventplus::Event& event) :
    bus(bus),
    event(event),
    hardShutdownMatch(bus,
                      "type='signal',member='PropertiesChanged',"
                      "path_namespace='/xyz/openbmc_project/sensors',"
                      "arg0='" +
                          shutdownInterfaces.at(ShutdownType::soft) + "'",
                      std::bind(&ShutdownAlarmMonitor::propertiesChanged, this,
                                std::placeholders::_1)),
    softShutdownMatch(bus,
                      "type='signal',member='PropertiesChanged',"
                      "path_namespace='/xyz/openbmc_project/sensors',"
                      "arg0='" +
                          shutdownInterfaces.at(ShutdownType::hard) + "'",
                      std::bind(&ShutdownAlarmMonitor::propertiesChanged, this,
                                std::placeholders::_1)),
    _powerState(std::make_unique<PGoodState>(
        bus, std::bind(&ShutdownAlarmMonitor::powerStateChanged, this,
                       std::placeholders::_1)))
{
    findAlarms();

    if (_powerState->isPowerOn())
    {
        checkAlarms();
    }
}

void ShutdownAlarmMonitor::findAlarms()
{
    // Find all shutdown threshold ifaces currently on D-Bus.
    for (const auto& [shutdownType, interface] : shutdownInterfaces)
    {
        auto paths = SDBusPlus::getSubTreePathsRaw(bus, "/", interface, 0);

        std::for_each(
            paths.begin(), paths.end(), [this, shutdownType](const auto& path) {
                alarms.emplace(AlarmKey{path, shutdownType, AlarmType::high},
                               nullptr);
                alarms.emplace(AlarmKey{path, shutdownType, AlarmType::low},
                               nullptr);
            });
    }
}

void ShutdownAlarmMonitor::checkAlarms()
{
    for (auto& [alarmKey, timer] : alarms)
    {
        const auto& [sensorPath, shutdownType, alarmType] = alarmKey;
        const auto& interface = shutdownInterfaces.at(shutdownType);
        auto propertyName = alarmProperties.at(shutdownType).at(alarmType);
        bool value;

        try
        {
            value = SDBusPlus::getProperty<bool>(bus, sensorPath, interface,
                                                 propertyName);
        }
        catch (const DBusServiceError& e)
        {
            // The sensor isn't on D-Bus anymore
            log<level::INFO>(fmt::format("No {} interface on {} anymore.",
                                         interface, sensorPath)
                                 .c_str());
            continue;
        }

        checkAlarm(value, alarmKey);
    }
}

void ShutdownAlarmMonitor::propertiesChanged(
    sdbusplus::message::message& message)
{
    // check the values
}

void ShutdownAlarmMonitor::checkAlarm(bool value, const AlarmKey& alarmKey)
{
    // start or stop a timer possibly
}

void ShutdownAlarmMonitor::powerStateChanged(bool powerStateOn)
{
    if (powerStateOn)
    {
        checkAlarms();
    }
    else
    {
        // Cancel and delete all timers
    }
}

} // namespace sensor::monitor
