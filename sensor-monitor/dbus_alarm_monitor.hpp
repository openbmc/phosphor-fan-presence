#pragma once
#include "alarm_checker.hpp"
#include "alarm_handlers.hpp"
#include "types.hpp"

#include <sdbusplus/bus/match.hpp>

#include <map>

namespace sensor::monitor
{

/**
 * @class DbusAlarmMonitor
 *
 * This class watches all property changing of the D-Bus interfaces:
 *   xyz.openbmc_project.Sensor.Threshold.SoftShutdown
 *   xyz.openbmc_project.Sensor.Threshold.HardShutdown
 *
 * If they trip, the code will handle the alarm with AlarmChecker.
 */
class DbusAlarmMonitor
{
  public:
    /**
     * @brief Constructor
     *
     * @param[in] alarmChecker - The AlarmChecker object
     */
    DbusAlarmMonitor(AlarmChecker& alarmChecker);

    /**
     * @brief Start watching the alarm of specific sensor
     *
     * @param[in] sensorPath - The sensor's path on dbus
     * @param[in] alarmType - The type of alarm to watch
     */
    void watchSensorAlarm(const std::string& sensorPath, AlarmType alarmType);

    /**
     * @brief Stop watching the alarm of specific sensor
     *
     * @param[in] sensorPath - The sensor's path on dbus
     * @param[in] alarmType - The type of alarm to stop watching
     */
    void stopWatchAlarm(const std::string& sensorPath, AlarmType alarmType);

  private:
    /**
     * @brief The propertiesChanged handled for an alarm
     *
     * @param[in] message - The message of sdbusplus
     */
    void propertiesChanged(sdbusplus::message_t& message);
    /**
     * @brief Get the dbus interface name of the alarmType
     *
     * @param[in] alarmType - The type of alarm to find
     */
    std::string getAlarmTypeInterface(AlarmType alarmType);

    /**
     * @brief Transform the interface name into AlarmType object
     *
     * @param[in] interface - The interface name of AlarmType
     */
    std::optional<AlarmType> getAlarmType(const std::string& interface) const;

    /**
     * @brief The match for properties changing on the watched alarms
     */
    std::map<std::pair<std::string, AlarmType>,
             std::shared_ptr<sdbusplus::bus::match_t>>
        matches;

    /**
     * @brief The AlarmChecker
     */
    AlarmChecker& alarmChecker;

    /**
     * @brief The match for properties changing on the HardShutdown
     *        interface.
     */
    sdbusplus::bus::match_t hardShutdownMatch;

    /**
     * @brief The match for properties changing on the SoftShutdown
     *        interface.
     */
    sdbusplus::bus::match_t softShutdownMatch;
};

} // namespace sensor::monitor
