#pragma once
#include "alarm_checker.hpp"
#include "alarm_handlers.hpp"
#include "types.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

#include <map>

namespace sensor::monitor
{
namespace sdbusRule = sdbusplus::bus::match::rules;
using namespace phosphor::logging;

const std::map<AlarmType, std::string> alarmInterfaces{
    {AlarmType::hardShutdown,
     "xyz.openbmc_project.Sensor.Threshold.HardShutdown"},
    {AlarmType::softShutdown,
     "xyz.openbmc_project.Sensor.Threshold.SoftShutdown"},
    {AlarmType::critical, "xyz.openbmc_project.Sensor.Threshold.Critical"},
    {AlarmType::warning, "xyz.openbmc_project.Sensor.Threshold.Warning"}};

class DbusAlarmMonitor
{
  public:
    DbusAlarmMonitor(sdbusplus::bus_t& bus, AlarmChecker& alarmChecker) :
        _bus(bus), _alarmChecker(alarmChecker),
        _hardShutdownMatch(
            bus,
            sdbusRule::type::signal() + sdbusRule::member("PropertiesChanged") +
                sdbusRule::path_namespace("/xyz/openbmc_project/sensors") +
                sdbusRule::argN(0, alarmInterfaces.at(AlarmType::hardShutdown)),
            std::bind(&DbusAlarmMonitor::propertiesChanged, this,
                      std::placeholders::_1)),
        _softShutdownMatch(
            bus,
            sdbusRule::type::signal() + sdbusRule::member("PropertiesChanged") +
                sdbusRule::path_namespace("/xyz/openbmc_project/sensors") +
                sdbusRule::argN(0, alarmInterfaces.at(AlarmType::softShutdown)),
            std::bind(&DbusAlarmMonitor::propertiesChanged, this,
                      std::placeholders::_1))
    {}

    void watchSensorAlarm(std::string sensorPath, AlarmType alarmType)
    {
        auto match = std::make_shared<sdbusplus::bus::match_t>(
            _bus,
            sdbusRule::type::signal() + sdbusRule::member("PropertiesChanged") +
                sdbusRule::path(sensorPath) +
                sdbusRule::interface("org.freedesktop.DBus.Properties") +
                sdbusRule::argN(0, getAlarmTypeInterface(alarmType)),
            std::bind(&DbusAlarmMonitor::propertiesChanged, this,
                      std::placeholders::_1));

        _matches.emplace(std::make_pair(sensorPath, alarmType), match);

        return;
    }

    void stopWatchAlarm(std::string sensorPath, AlarmType alarmType)
    {
        auto identifier = std::make_pair(sensorPath, alarmType);
        auto it = _matches.find(identifier);
        if (it == _matches.end())
        {
            log<level::ERR>(
                fmt::format("The Alarm watch for {} cannot found", sensorPath)
                    .c_str());
            return;
        }

        _matches.erase(identifier);

        return;
    }

  private:
    std::string getAlarmTypeInterface(AlarmType alarmType)
    {
        return alarmInterfaces.at(alarmType);
    }

    std::optional<AlarmType> getAlarmType(const std::string& interface) const
    {
        auto it = std::find_if(
            alarmInterfaces.begin(), alarmInterfaces.end(),
            [interface](const auto& a) { return a.second == interface; });

        if (it == alarmInterfaces.end())
        {
            return std::nullopt;
        }

        return it->first;
    }

    void propertiesChanged(sdbusplus::message_t& message)
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

        _alarmChecker.checkAlarm(*type, sensorPath, properties);
    }

    sdbusplus::bus_t& _bus;

    std::map<std::pair<std::string, AlarmType>,
             std::shared_ptr<sdbusplus::bus::match_t>>
        _matches;

    AlarmChecker& _alarmChecker;

    sdbusplus::bus::match_t _hardShutdownMatch;

    sdbusplus::bus::match_t _softShutdownMatch;
};

} // namespace sensor::monitor
