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

const std::map<ShutdownType, std::chrono::milliseconds> shutdownDelays{
    {ShutdownType::hard,
     std::chrono::milliseconds{SHUTDOWN_ALARM_HARD_SHUTDOWN_DELAY_MS}},
    {ShutdownType::soft,
     std::chrono::milliseconds{SHUTDOWN_ALARM_SOFT_SHUTDOWN_DELAY_MS}}};

constexpr auto systemdService = "org.freedesktop.systemd1";
constexpr auto systemdPath = "/org/freedesktop/systemd1";
constexpr auto systemdMgrIface = "org.freedesktop.systemd1.Manager";
constexpr auto valueInterface = "xyz.openbmc_project.Sensor.Value";
constexpr auto valueProperty = "Value";

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
        catch (const DBusError& e)
        {
            log<level::ERR>(
                fmt::format("Failed reading threshold properties from {}: {}",
                            sensorPath, e.what())
                    .c_str());
            throw;
        }

        checkAlarm(value, alarmKey);
    }
}

void ShutdownAlarmMonitor::propertiesChanged(
    sdbusplus::message::message& message)
{
    std::map<std::string, std::variant<bool>> properties;
    std::string interface;

    if (!_powerState->isPowerOn())
    {
        return;
    }

    message.read(interface, properties);

    auto type = getShutdownType(interface);
    if (!type)
    {
        return;
    }

    std::string sensorPath = message.get_path();

    const auto& lowAlarmName = alarmProperties.at(*type).at(AlarmType::low);
    if (properties.count(lowAlarmName) > 0)
    {
        AlarmKey alarmKey{sensorPath, *type, AlarmType::low};
        auto alarm = alarms.find(alarmKey);
        if (alarm == alarms.end())
        {
            alarms.emplace(alarmKey, nullptr);
        }
        checkAlarm(std::get<bool>(properties.at(lowAlarmName)), alarmKey);
    }

    const auto& highAlarmName = alarmProperties.at(*type).at(AlarmType::high);
    if (properties.count(highAlarmName) > 0)
    {
        AlarmKey alarmKey{sensorPath, *type, AlarmType::high};
        auto alarm = alarms.find(alarmKey);
        if (alarm == alarms.end())
        {
            alarms.emplace(alarmKey, nullptr);
        }
        checkAlarm(std::get<bool>(properties.at(highAlarmName)), alarmKey);
    }
}

void ShutdownAlarmMonitor::checkAlarm(bool value, const AlarmKey& alarmKey)
{
    auto alarm = alarms.find(alarmKey);
    if (alarm == alarms.end())
    {
        return;
    }

    // Start or stop the timer if necessary.
    auto& timer = alarm->second;
    if (value)
    {
        if (!timer)
        {
            startTimer(alarmKey);
        }
    }
    else
    {
        if (timer)
        {
            stopTimer(alarmKey);
        }
    }
}

void ShutdownAlarmMonitor::startTimer(const AlarmKey& alarmKey)
{
    const auto& [sensorPath, shutdownType, alarmType] = alarmKey;
    const auto& propertyName = alarmProperties.at(shutdownType).at(alarmType);
    std::chrono::milliseconds shutdownDelay{shutdownDelays.at(shutdownType)};
    std::optional<double> value;

    auto alarm = alarms.find(alarmKey);
    if (alarm == alarms.end())
    {
        throw std::runtime_error("Couldn't find alarm inside startTimer");
    }

    try
    {
        value = SDBusPlus::getProperty<double>(bus, sensorPath, valueInterface,
                                               valueProperty);
    }
    catch (const DBusServiceError& e)
    {
        // If the sensor was just added, the Value interface for it may
        // not be in the mapper yet.  This could only happen if the sensor
        // application was started with power up and the value exceeded the
        // threshold immediately.
    }

    log<level::INFO>(
        fmt::format("Starting {}ms {} shutdown timer due to sensor {} value {}",
                    shutdownDelay.count(), propertyName, sensorPath, *value)
            .c_str());

    auto& timer = alarm->second;

    timer = std::make_unique<
        sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>>(
        event, std::bind(&ShutdownAlarmMonitor::timerExpired, this, alarmKey));

    timer->restartOnce(shutdownDelay);
}

void ShutdownAlarmMonitor::stopTimer(const AlarmKey& alarmKey)
{
    const auto& [sensorPath, shutdownType, alarmType] = alarmKey;
    const auto& propertyName = alarmProperties.at(shutdownType).at(alarmType);

    auto value = SDBusPlus::getProperty<double>(bus, sensorPath, valueInterface,
                                                valueProperty);

    auto alarm = alarms.find(alarmKey);
    if (alarm == alarms.end())
    {
        throw std::runtime_error("Couldn't find alarm inside stopTimer");
    }

    log<level::INFO>(
        fmt::format("Stopping {} shutdown timer due to sensor {} value {}",
                    propertyName, sensorPath, value)
            .c_str());

    auto& timer = alarm->second;
    timer->setEnabled(false);
    timer.reset();
}

void ShutdownAlarmMonitor::timerExpired(const AlarmKey& alarmKey)
{
    const auto& [sensorPath, shutdownType, alarmType] = alarmKey;
    const auto& propertyName = alarmProperties.at(shutdownType).at(alarmType);

    log<level::ERR>(
        fmt::format(
            "The {} shutdown timer expired for sensor {}, shutting down",
            propertyName, sensorPath)
            .c_str());

    SDBusPlus::callMethod(systemdService, systemdPath, systemdMgrIface,
                          "StartUnit", "obmc-chassis-hard-poweroff@0.target",
                          "replace");
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
        std::for_each(alarms.begin(), alarms.end(), [](auto& alarm) {
            auto& timer = alarm.second;
            if (timer)
            {
                timer->setEnabled(false);
                timer.reset();
            }
        });
    }
}

std::optional<ShutdownType>
    ShutdownAlarmMonitor::getShutdownType(const std::string& interface) const
{
    auto it = std::find_if(
        shutdownInterfaces.begin(), shutdownInterfaces.end(),
        [interface](const auto& a) { return a.second == interface; });

    if (it == shutdownInterfaces.end())
    {
        return std::nullopt;
    }

    return it->first;
}

} // namespace sensor::monitor
