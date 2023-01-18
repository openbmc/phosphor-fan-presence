#include "alarm_checker.hpp"

#include <fmt/format.h>

#include <phosphor-logging/log.hpp>

namespace sensor::monitor
{
using namespace phosphor::logging;
namespace util = phosphor::fan::util;

const std::map<AlarmType, std::string> alarmInterfaces{
    {AlarmType::hardShutdown,
     "xyz.openbmc_project.Sensor.Threshold.HardShutdown"},
    {AlarmType::softShutdown,
     "xyz.openbmc_project.Sensor.Threshold.SoftShutdown"},
    {AlarmType::critical, "xyz.openbmc_project.Sensor.Threshold.Critical"},
    {AlarmType::warning, "xyz.openbmc_project.Sensor.Threshold.Warning"}};

const std::map<AlarmType, std::map<AlarmDirection, std::string>>
    alarmProperties{{AlarmType::hardShutdown,
                     {{AlarmDirection::low, "HardShutdownAlarmLow"},
                      {AlarmDirection::high, "HardShutdownAlarmHigh"}}},
                    {AlarmType::softShutdown,
                     {{AlarmDirection::low, "SoftShutdownAlarmLow"},
                      {AlarmDirection::high, "SoftShutdownAlarmHigh"}}},
                    {AlarmType::critical,
                     {{AlarmDirection::low, "CriticalAlarmLow"},
                      {AlarmDirection::high, "CriticalAlarmHigh"}}},
                    {AlarmType::warning,
                     {{AlarmDirection::low, "WarningAlarmLow"},
                      {AlarmDirection::high, "WarningAlarmHigh"}}}};

AlarmChecker::AlarmChecker(
    std::map<AlarmKey, std::unique_ptr<sdeventplus::utility::Timer<
                           sdeventplus::ClockId::Monotonic>>>& alarms,
    std::shared_ptr<phosphor::fan::PowerState> powerState) :
    alarms(alarms),
    powerState(powerState)
{}

void AlarmChecker::checkAlarms()
{
    for (const auto& [alarmKey, timer] : alarms)
    {
        const auto& [sensorPath, alarmType, alarmDirection] = alarmKey;
        const auto& interface = alarmInterfaces.at(alarmType);
        auto propertyName = alarmProperties.at(alarmType).at(alarmDirection);
        bool isTriggered;
        try
        {
            isTriggered = util::SDBusPlus::getProperty<bool>(
                util::SDBusPlus::getBus(), sensorPath, interface, propertyName);
        }
        catch (const util::DBusServiceError& e)
        {
            // The sensor isn't on D-Bus anymore
            log<level::INFO>(fmt::format("No {} interface on {} anymore.",
                                         interface, sensorPath)
                                 .c_str());
            continue;
        }

        std::map<std::string, std::variant<bool>> properties;
        properties.emplace(propertyName, isTriggered);

        checkAlarm(alarmType, sensorPath, properties);
    }
}

void AlarmChecker::checkAlarm(
    AlarmType alarmType, const std::string& sensorPath,
    const std::map<std::string, std::variant<bool>>& properties)
{
    auto alarmHandler = obtainAlarmHandler(alarmType);
    alarmHandler->checkAlarm(sensorPath, properties);
}

std::unique_ptr<AlarmHandler>
    AlarmChecker::obtainAlarmHandler(AlarmType alarmType)
{
    switch (alarmType)
    {
        case AlarmType::hardShutdown:
        case AlarmType::softShutdown:
            return std::make_unique<ProtectionAlarmHandler>(alarms, alarmType,
                                                            powerState);
        case AlarmType::critical:
        case AlarmType::warning:
            return std::make_unique<RecoveryAlarmHandler>(alarms, alarmType);
        default:
            throw std::runtime_error("Unsupported alarmType");
    }
}
} // namespace sensor::monitor
