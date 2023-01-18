#include "config.h"

#include "dbus_alarm_monitor.hpp"

#include <phosphor-logging/log.hpp>

namespace sensor::monitor
{
namespace sdbusRule = sdbusplus::bus::match::rules;
namespace util = phosphor::fan::util;
using namespace phosphor::logging;

const std::map<AlarmType, std::string> alarmInterfaces{
    {AlarmType::hardShutdown,
     "xyz.openbmc_project.Sensor.Threshold.HardShutdown"},
    {AlarmType::softShutdown,
     "xyz.openbmc_project.Sensor.Threshold.SoftShutdown"},
    {AlarmType::critical, "xyz.openbmc_project.Sensor.Threshold.Critical"},
    {AlarmType::warning, "xyz.openbmc_project.Sensor.Threshold.Warning"}};

DbusAlarmMonitor::DbusAlarmMonitor(AlarmChecker& alarmChecker) :
    alarmChecker(alarmChecker),
    hardShutdownMatch(
        util::SDBusPlus::getBus(),
        sdbusRule::type::signal() + sdbusRule::member("PropertiesChanged") +
            sdbusRule::path_namespace("/xyz/openbmc_project/sensors") +
            sdbusRule::argN(0, alarmInterfaces.at(AlarmType::hardShutdown)),
        std::bind(&DbusAlarmMonitor::propertiesChanged, this,
                  std::placeholders::_1)),
    softShutdownMatch(
        util::SDBusPlus::getBus(),
        sdbusRule::type::signal() + sdbusRule::member("PropertiesChanged") +
            sdbusRule::path_namespace("/xyz/openbmc_project/sensors") +
            sdbusRule::argN(0, alarmInterfaces.at(AlarmType::softShutdown)),
        std::bind(&DbusAlarmMonitor::propertiesChanged, this,
                  std::placeholders::_1))
{}

void DbusAlarmMonitor::propertiesChanged(sdbusplus::message_t& message)
{
    std::map<std::string, std::variant<bool>> properties;
    std::string interface;

    message.read(interface, properties);

    auto type = getAlarmType(interface);
    if (!type)
    {
        log<level::ERR>(
            fmt::format("Cannot find AlarmType for {}", interface).c_str());
        return;
    }
    std::string sensorPath = message.get_path();

    alarmChecker.checkAlarm(*type, sensorPath, properties);
}

void DbusAlarmMonitor::watchSensorAlarm(const std::string& sensorPath,
                                        AlarmType alarmType)
{
    auto match = std::make_shared<sdbusplus::bus::match_t>(
        util::SDBusPlus::getBus(),
        sdbusRule::type::signal() + sdbusRule::member("PropertiesChanged") +
            sdbusRule::path(sensorPath) +
            sdbusRule::interface("org.freedesktop.DBus.Properties") +
            sdbusRule::argN(0, getAlarmTypeInterface(alarmType)),
        std::bind(&DbusAlarmMonitor::propertiesChanged, this,
                  std::placeholders::_1));

    matches.emplace(std::make_pair(sensorPath, alarmType), match);
}

void DbusAlarmMonitor::stopWatchAlarm(const std::string& sensorPath,
                                      AlarmType alarmType)
{
    auto identifier = std::make_pair(sensorPath, alarmType);
    auto it = matches.find(identifier);
    if (it == matches.end())
    {
        auto transformToString = [](AlarmType& alarmType) {
            switch (alarmType)
            {
                case AlarmType::hardShutdown:
                    return "HardShutdown";
                case AlarmType::softShutdown:
                    return "SoftShutdown";
                case AlarmType::critical:
                    return "Critical";
                case AlarmType::warning:
                    return "Warning";
                default:
                    return "Not found";
            }
        };

        log<level::ERR>(fmt::format("The Alarm watch for {}, {} cannot found",
                                    sensorPath, transformToString(alarmType))
                            .c_str());
        return;
    }

    matches.erase(identifier);
}

std::string DbusAlarmMonitor::getAlarmTypeInterface(AlarmType alarmType)
{
    return alarmInterfaces.at(alarmType);
}

std::optional<AlarmType>
    DbusAlarmMonitor::getAlarmType(const std::string& interface) const
{
    auto it = std::find_if(alarmInterfaces.begin(), alarmInterfaces.end(),
                           [interface](const auto& a) {
        return a.second == interface;
    });

    if (it == alarmInterfaces.end())
    {
        return std::nullopt;
    }

    return it->first;
}

} // namespace sensor::monitor
