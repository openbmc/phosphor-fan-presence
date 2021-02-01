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
#include "threshold_alarm_logger.hpp"

#include "sdbusplus.hpp"

#include <fmt/format.h>

#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Logging/Entry/server.hpp>

namespace sensor::monitor
{

using namespace sdbusplus::xyz::openbmc_project::Logging::server;
using namespace phosphor::logging;
using namespace phosphor::fan::util;

const std::string warningInterface =
    "xyz.openbmc_project.Sensor.Threshold.Warning";
const std::string criticalInterface =
    "xyz.openbmc_project.Sensor.Threshold.Critical";
const std::string perfLossInterface =
    "xyz.openbmc_project.Sensor.Threshold.PerformanceLoss";

using ErrorData = std::tuple<ErrorName, Entry::Level>;

/**
 * Map of threshold interfaces and alarm properties and values to error data.
 */
const std::map<InterfaceName, std::map<PropertyName, std::map<bool, ErrorData>>>
    thresholdData{

        {warningInterface,
         {{"WarningAlarmHigh",
           {{true, ErrorData{"WarningHigh", Entry::Level::Warning}},
            {false,
             ErrorData{"WarningHighClear", Entry::Level::Informational}}}},
          {"WarningAlarmLow",
           {{true, ErrorData{"WarningLow", Entry::Level::Warning}},
            {false,
             ErrorData{"WarningLowClear", Entry::Level::Informational}}}}}},

        {criticalInterface,
         {{"CriticalAlarmHigh",
           {{true, ErrorData{"CriticalHigh", Entry::Level::Critical}},
            {false,
             ErrorData{"CriticalHighClear", Entry::Level::Informational}}}},
          {"CriticalAlarmLow",
           {{true, ErrorData{"CriticalLow", Entry::Level::Critical}},
            {false,
             ErrorData{"CriticalLowClear", Entry::Level::Informational}}}}}},

        {perfLossInterface,
         {{"PerfLossAlarmHigh",
           {{true, ErrorData{"PerfLossHigh", Entry::Level::Warning}},
            {false,
             ErrorData{"PerfLossHighClear", Entry::Level::Informational}}}},
          {"PerfLossAlarmLow",
           {{true, ErrorData{"PerfLossLow", Entry::Level::Warning}},
            {false,
             ErrorData{"PerfLossLowClear", Entry::Level::Informational}}}}}}};

ThresholdAlarmLogger::ThresholdAlarmLogger(sdbusplus::bus::bus& bus,
                                           sdeventplus::Event& event) :
    bus(bus),
    event(event),
    warningMatch(bus,
                 "type='signal',member='PropertiesChanged',"
                 "path_namespace='/xyz/openbmc_project/sensors',"
                 "arg0='" +
                     warningInterface + "'",
                 std::bind(&ThresholdAlarmLogger::propertiesChanged, this,
                           std::placeholders::_1)),
    criticalMatch(bus,
                  "type='signal',member='PropertiesChanged',"
                  "path_namespace='/xyz/openbmc_project/sensors',"
                  "arg0='" +
                      criticalInterface + "'",
                  std::bind(&ThresholdAlarmLogger::propertiesChanged, this,
                            std::placeholders::_1)),
    perfLossMatch(bus,
                  "type='signal',member='PropertiesChanged',"
                  "path_namespace='/xyz/openbmc_project/sensors',"
                  "arg0='" +
                      perfLossInterface + "'",
                  std::bind(&ThresholdAlarmLogger::propertiesChanged, this,
                            std::placeholders::_1))
{
    // check for any currently asserted threshold alarms
    std::for_each(thresholdData.begin(), thresholdData.end(),
                  [this](const auto& thresholdinterface) {
                      const auto& interface = thresholdinterface.first;
                      auto paths = SDBusPlus::getSubTreePathsRaw(this->bus, "/",
                                                                 interface, 0);
                      std::for_each(paths.begin(), paths.end(),
                                    [interface, this](const auto& sensorPath) {
                                        checkThresholds(interface, sensorPath);
                                    });
                  });
}

void ThresholdAlarmLogger::propertiesChanged(sdbusplus::message::message& msg)
{
    std::map<std::string, std::variant<bool>> properties;
    std::string sensorPath = msg.get_path();
    std::string interface;

    msg.read(interface, properties);

    auto alarmProperties = thresholdData.find(interface);
    if (alarmProperties == thresholdData.end())
    {
        return;
    }

    for (const auto& [propertyName, propertyValue] : properties)
    {
        if (alarmProperties->second.find(propertyName) !=
            alarmProperties->second.end())
        {
            // If this is the first time we've seen this alarm, then
            // assume it was off before so it doesn't create an event
            // log for a value of false.

            InterfaceKey key{sensorPath, interface};
            if (alarms.find(key) == alarms.end())
            {
                alarms[key][propertyName] = false;
            }

            // Check if the value changed from what was there before.
            auto alarmValue = std::get<bool>(propertyValue);
            if (alarmValue != alarms[key][propertyName])
            {
                alarms[key][propertyName] = alarmValue;
                createEventLog(sensorPath, interface, propertyName, alarmValue);
            }
        }
    }
}

void ThresholdAlarmLogger::checkThresholds(const std::string& interface,
                                           const std::string& sensorPath)
{
    auto properties = thresholdData.find(interface);
    if (properties == thresholdData.end())
    {
        return;
    }

    for (const auto& [property, unused] : properties->second)
    {
        try
        {
            auto alarmValue = SDBusPlus::getProperty<bool>(bus, sensorPath,
                                                           interface, property);

            alarms[InterfaceKey(interface, sensorPath)][property] = alarmValue;

            // This is just for checking alarms on startup,
            // so only look for active alarms.
            if (alarmValue)
            {
                createEventLog(sensorPath, interface, property, alarmValue);
            }
        }
        catch (const DBusError& e)
        {
            log<level::ERR>(
                fmt::format("Failed reading sensor threshold properties: {}",
                            e.what())
                    .c_str());
            continue;
        }
    }
}

void ThresholdAlarmLogger::createEventLog(const std::string& sensorPath,
                                          const std::string& interface,
                                          const std::string& alarmProperty,
                                          bool alarmValue)
{
    // TODO
}

} // namespace sensor::monitor
