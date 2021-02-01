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

namespace sensor::monitor
{

const std::string warningInterface =
    "xyz.openbmc_project.Sensor.Threshold.Warning";
const std::string criticalInterface =
    "xyz.openbmc_project.Sensor.Threshold.Critical";
const std::string perfLossInterface =
    "xyz.openbmc_project.Sensor.Threshold.PerformanceLoss";

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
{}

void ThresholdAlarmLogger::propertiesChanged(sdbusplus::message::message& msg)
{
    // TODO
}

} // namespace sensor::monitor
