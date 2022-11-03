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
#include <unistd.h>

#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Logging/Entry/server.hpp>

namespace sensor::monitor
{

using namespace sdbusplus::xyz::openbmc_project::Logging::server;
using namespace phosphor::logging;
using namespace phosphor::fan;
using namespace phosphor::fan::util;

const std::string warningInterface =
    "xyz.openbmc_project.Sensor.Threshold.Warning";
const std::string criticalInterface =
    "xyz.openbmc_project.Sensor.Threshold.Critical";
const std::string perfLossInterface =
    "xyz.openbmc_project.Sensor.Threshold.PerformanceLoss";
constexpr auto loggingService = "xyz.openbmc_project.Logging";
constexpr auto loggingPath = "/xyz/openbmc_project/logging";
constexpr auto loggingCreateIface = "xyz.openbmc_project.Logging.Create";
constexpr auto errorNameBase = "xyz.openbmc_project.Sensor.Threshold.Error.";
constexpr auto valueInterface = "xyz.openbmc_project.Sensor.Value";
constexpr auto assocInterface = "xyz.openbmc_project.Association";

const std::vector<std::string> thresholdIfaceNames{
    warningInterface, criticalInterface, perfLossInterface};

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

ThresholdAlarmLogger::ThresholdAlarmLogger(
    sdbusplus::bus_t& bus, sdeventplus::Event& event,
    std::shared_ptr<PowerState> powerState) :
    bus(bus),
    event(event), _powerState(std::move(powerState)),
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
                            std::placeholders::_1)),
    ifacesRemovedMatch(bus,
                       "type='signal',member='InterfacesRemoved',arg0path="
                       "'/xyz/openbmc_project/sensors/'",
                       std::bind(&ThresholdAlarmLogger::interfacesRemoved, this,
                                 std::placeholders::_1)),
    ifacesAddedMatch(bus,
                     "type='signal',member='InterfacesAdded',arg0path="
                     "'/xyz/openbmc_project/sensors/'",
                     std::bind(&ThresholdAlarmLogger::interfacesAdded, this,
                               std::placeholders::_1))
{
    _powerState->addCallback("thresholdMon",
                             std::bind(&ThresholdAlarmLogger::powerStateChanged,
                                       this, std::placeholders::_1));

    // check for any currently asserted threshold alarms
    std::for_each(
        thresholdData.begin(), thresholdData.end(),
        [this](const auto& thresholdInterface) {
            const auto& interface = thresholdInterface.first;
            auto objects =
                SDBusPlus::getSubTreeRaw(this->bus, "/", interface, 0);
            std::for_each(objects.begin(), objects.end(),
                          [interface, this](const auto& object) {
                              const auto& path = object.first;
                              const auto& service =
                                  object.second.begin()->first;
                              checkThresholds(interface, path, service);
                          });
        });
}

void ThresholdAlarmLogger::propertiesChanged(sdbusplus::message_t& msg)
{
    std::map<std::string, std::variant<bool>> properties;
    std::string sensorPath = msg.get_path();
    std::string interface;

    msg.read(interface, properties);

    checkProperties(sensorPath, interface, properties);
}

void ThresholdAlarmLogger::interfacesRemoved(sdbusplus::message_t& msg)
{
    sdbusplus::message::object_path path;
    std::vector<std::string> interfaces;

    msg.read(path, interfaces);

    for (const auto& interface : interfaces)
    {
        if (std::find(thresholdIfaceNames.begin(), thresholdIfaceNames.end(),
                      interface) != thresholdIfaceNames.end())
        {
            alarms.erase(InterfaceKey{path, interface});
        }
    }
}

void ThresholdAlarmLogger::interfacesAdded(sdbusplus::message_t& msg)
{
    sdbusplus::message::object_path path;
    std::map<std::string, std::map<std::string, std::variant<bool>>> interfaces;

    msg.read(path, interfaces);

    for (const auto& [interface, properties] : interfaces)
    {
        if (std::find(thresholdIfaceNames.begin(), thresholdIfaceNames.end(),
                      interface) != thresholdIfaceNames.end())
        {
            checkProperties(path, interface, properties);
        }
    }
}

void ThresholdAlarmLogger::checkProperties(
    const std::string& sensorPath, const std::string& interface,
    const std::map<std::string, std::variant<bool>>& properties)
{
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

                if (_powerState->isPowerOn())
                {
                    createEventLog(sensorPath, interface, propertyName,
                                   alarmValue);
                }
            }
        }
    }
}

void ThresholdAlarmLogger::checkThresholds(const std::string& interface,
                                           const std::string& sensorPath,
                                           const std::string& service)
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
            auto alarmValue = SDBusPlus::getProperty<bool>(
                bus, service, sensorPath, interface, property);
            alarms[InterfaceKey(sensorPath, interface)][property] = alarmValue;

            // This is just for checking alarms on startup,
            // so only look for active alarms.
            if (alarmValue && _powerState->isPowerOn())
            {
                createEventLog(sensorPath, interface, property, alarmValue);
            }
        }
        catch (const sdbusplus::exception_t& e)
        {
            // Sensor daemons that get their direction from entity manager
            // may only be putting either the high alarm or low alarm on
            // D-Bus, not both.
            continue;
        }
    }
}

void ThresholdAlarmLogger::createEventLog(const std::string& sensorPath,
                                          const std::string& interface,
                                          const std::string& alarmProperty,
                                          bool alarmValue)
{
    std::map<std::string, std::string> ad;

    auto type = getSensorType(sensorPath);
    if (skipSensorType(type))
    {
        return;
    }

    auto it = thresholdData.find(interface);
    if (it == thresholdData.end())
    {
        return;
    }

    auto properties = it->second.find(alarmProperty);
    if (properties == it->second.end())
    {
        log<level::INFO>(
            fmt::format("Could not find {} in threshold alarms map",
                        alarmProperty)
                .c_str());
        return;
    }

    ad.emplace("SENSOR_NAME", sensorPath);
    ad.emplace("_PID", std::to_string(getpid()));

    try
    {
        auto sensorValue = SDBusPlus::getProperty<double>(
            bus, sensorPath, valueInterface, "Value");

        ad.emplace("SENSOR_VALUE", std::to_string(sensorValue));

        log<level::INFO>(
            fmt::format("Threshold Event {} {} = {} (sensor value {})",
                        sensorPath, alarmProperty, alarmValue, sensorValue)
                .c_str());
    }
    catch (const DBusServiceError& e)
    {
        // If the sensor was just added, the Value interface for it may
        // not be in the mapper yet.  This could only happen if the sensor
        // application was started up after this one and the value exceeded the
        // threshold immediately.
        log<level::INFO>(fmt::format("Threshold Event {} {} = {}", sensorPath,
                                     alarmProperty, alarmValue)
                             .c_str());
    }

    auto callout = getCallout(sensorPath);
    if (!callout.empty())
    {
        ad.emplace("CALLOUT_INVENTORY_PATH", callout);
    }

    auto errorData = properties->second.find(alarmValue);

    // Add the base error name and the sensor type (like Temperature) to the
    // error name that's in the thresholdData name to get something like
    // xyz.openbmc_project.Sensor.Threshold.Error.TemperatureWarningHigh
    const auto& [name, severity] = errorData->second;
    type.front() = toupper(type.front());
    std::string errorName = errorNameBase + type + name;

    SDBusPlus::callMethod(loggingService, loggingPath, loggingCreateIface,
                          "Create", errorName, convertForMessage(severity), ad);
}

std::string ThresholdAlarmLogger::getSensorType(std::string sensorPath)
{
    auto pos = sensorPath.find_last_of('/');
    if ((sensorPath.back() == '/') || (pos == std::string::npos))
    {
        log<level::ERR>(
            fmt::format("Cannot get sensor type from sensor path {}",
                        sensorPath)
                .c_str());
        throw std::runtime_error("Invalid sensor path");
    }

    sensorPath = sensorPath.substr(0, pos);
    return sensorPath.substr(sensorPath.find_last_of('/') + 1);
}

bool ThresholdAlarmLogger::skipSensorType(const std::string& type)
{
    return (type == "utilization");
}

std::string ThresholdAlarmLogger::getCallout(const std::string& sensorPath)
{
    const std::array<std::string, 2> assocTypes{"inventory", "chassis"};

    // Different implementations handle the association to the FRU
    // differently:
    //  * phosphor-inventory-manager uses the 'inventory' association
    //    to point to the FRU.
    //  * dbus-sensors/entity-manager uses the 'chassis' association'.
    //  * For virtual sensors, no association.

    for (const auto& assocType : assocTypes)
    {
        auto assocPath = sensorPath + "/" + assocType;

        try
        {
            auto endpoints = SDBusPlus::getProperty<std::vector<std::string>>(
                bus, assocPath, assocInterface, "endpoints");

            if (!endpoints.empty())
            {
                return endpoints[0];
            }
        }
        catch (const DBusServiceError& e)
        {
            // The association doesn't exist
            continue;
        }
    }

    return std::string{};
}

void ThresholdAlarmLogger::powerStateChanged(bool powerStateOn)
{
    if (powerStateOn)
    {
        checkThresholds();
    }
}

void ThresholdAlarmLogger::checkThresholds()
{
    std::vector<InterfaceKey> toErase;

    for (const auto& [interfaceKey, alarmMap] : alarms)
    {
        for (const auto& [propertyName, alarmValue] : alarmMap)
        {
            if (alarmValue)
            {
                const auto& sensorPath = std::get<0>(interfaceKey);
                const auto& interface = std::get<1>(interfaceKey);
                std::string service;

                try
                {
                    // Check that the service that provides the alarm is still
                    // running, because if it died when the alarm was active
                    // there would be no indication of it unless we listened
                    // for NameOwnerChanged and tracked services, and this is
                    // easier.
                    service = SDBusPlus::getService(bus, sensorPath, interface);
                }
                catch (const DBusServiceError& e)
                {
                    // No longer on D-Bus delete the alarm entry
                    toErase.emplace_back(sensorPath, interface);
                }

                if (!service.empty())
                {
                    createEventLog(sensorPath, interface, propertyName,
                                   alarmValue);
                }
            }
        }
    }

    for (const auto& e : toErase)
    {
        alarms.erase(e);
    }
}

} // namespace sensor::monitor
