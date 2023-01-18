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

#include "domain_event_publisher.hpp"
#include "recovery_subscriber.hpp"

#include <fmt/format.h>
#include <unistd.h>

#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Logging/Entry/server.hpp>

namespace sensor::monitor
{
using namespace phosphor::logging;
using namespace phosphor::fan::util;
using namespace phosphor::fan;
using namespace sdbusplus::bus::match;
namespace fs = std::filesystem;

const std::map<AlarmType, std::string> alarmInterfaces{
    {AlarmType::hardShutdown,
     "xyz.openbmc_project.Sensor.Threshold.HardShutdown"},
    {AlarmType::softShutdown,
     "xyz.openbmc_project.Sensor.Threshold.SoftShutdown"},
    {AlarmType::critical, "xyz.openbmc_project.Sensor.Threshold.Critical"},
    {AlarmType::warning, "xyz.openbmc_project.Sensor.Threshold.Warning"}};

ShutdownAlarmMonitor::ShutdownAlarmMonitor(
    std::shared_ptr<PowerState> powerState) :
    _powerState(std::move(powerState)),
    _alarmChecker(_alarms, _powerState), _dbusAlarmMonitor(_alarmChecker)
{
    _powerState->addCallback("shutdownMon",
                             std::bind(&ShutdownAlarmMonitor::powerStateChanged,
                                       this, std::placeholders::_1));

#ifdef ENABLE_RECOVERY_DETECTION
    DomainEventPublisher::instance().subscribe(
        std::make_shared<RecoverySubscriber>(_dbusAlarmMonitor));
#endif

    std::vector<AlarmType> checkedAlarms = std::vector<AlarmType>{
        AlarmType::hardShutdown, AlarmType::softShutdown};
    findAlarms(checkedAlarms);

    if (_powerState->isPowerOn())
    {
        _alarmChecker.checkAlarms();

        // Get rid of any previous saved timestamps that don't
        // apply anymore.
        AlarmTimestamps::instance().prune(_alarms);
    }
    else
    {
        AlarmTimestamps::instance().clear();
    }
}

void ShutdownAlarmMonitor::findAlarms(std::vector<AlarmType> types)
{
    for (AlarmType& alarmType : types)
    {
        auto findDbusInterfaceOf = [](AlarmType theType) {
            const auto it = alarmInterfaces.find(theType);
            if (it == alarmInterfaces.end())
            {
                throw std::runtime_error("Couldn't find the interface in \
                    alarmInterfaces");
            }

            return it->second;
        };

        auto interface = findDbusInterfaceOf(alarmType);

        auto getAllDbusPathsWhichContain = [this](std::string& interface) {
            return SDBusPlus::getSubTreePathsRaw(SDBusPlus::getBus(), "/",
                                                 interface, 0);
        };

        auto sensorPaths = getAllDbusPathsWhichContain(interface);
        auto theAlarmType = alarmType;

        std::for_each(sensorPaths.begin(), sensorPaths.end(),
                      [this, theAlarmType](const auto& path) {
                          _alarms.emplace(AlarmKey{path, theAlarmType,
                                                   AlarmDirection::high},
                                          nullptr);
                          _alarms.emplace(
                              AlarmKey{path, theAlarmType, AlarmDirection::low},
                              nullptr);
                      });
    }
}

void ShutdownAlarmMonitor::powerStateChanged(bool powerStateOn)
{
    if (powerStateOn)
    {
        _alarmChecker.checkAlarms();
    }
    else
    {
        AlarmTimestamps::instance().clear();

        // Cancel and delete all timers
        std::for_each(_alarms.begin(), _alarms.end(), [](auto& alarm) {
            auto& timer = alarm.second;
            if (timer)
            {
                timer->setEnabled(false);
                timer.reset();
            }
        });
    }
}

} // namespace sensor::monitor
