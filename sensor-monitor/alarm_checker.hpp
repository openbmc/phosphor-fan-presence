#pragma once
#include "alarm_handlers.hpp"
#include "power_state.hpp"
#include "sdbusplus.hpp"
#include "types.hpp"

#include <sdeventplus/event.hpp>

#include <map>
#include <memory>

namespace sensor::monitor
{

/**
 * @class AlarmChecker
 *
 * This class checks sensor's alarm properties, use different
 * AlarmHandler to perform system protection and recovery
 * based on the type of Alarm.
 *
 * Currently, AlarmChecker only select AlarmHandler by the type
 * of alarm, such as HardShutdown, Critical, etc. If there are
 * more AlarmHandler are added with more complicated condition,
 * the obtainAlarmHandler() can be refactored to fit the requirement.
 */
class AlarmChecker
{
  public:
    AlarmChecker() = delete;
    ~AlarmChecker() = default;
    AlarmChecker(const AlarmChecker&) = delete;
    AlarmChecker& operator=(const AlarmChecker&) = delete;
    AlarmChecker(AlarmChecker&&) = delete;
    AlarmChecker& operator=(AlarmChecker&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] alarms - All alarms needs to be handled
     * @param[in] powerState - The PowerState object
     */
    AlarmChecker(
        std::map<AlarmKey, std::unique_ptr<sdeventplus::utility::Timer<
                               sdeventplus::ClockId::Monotonic>>>& alarms,
        std::shared_ptr<phosphor::fan::PowerState> powerState);

    /**
     * @brief Check an alarm of the sensor
     *
     * @param[in] alarmType = The type of the alarm
     * @param[in] sensorPath = The sensor's path on dbus
     * @param[in] properties = The changed properties of the alarm interface
     */
    void
        checkAlarm(AlarmType alarmType, const std::string& sensorPath,
                   const std::map<std::string, std::variant<bool>>& properties);

    /**
     * @brief Check all recorded alarms
     */
    void checkAlarms();

  private:
    /**
     * @brief Select corresponding AlarmHandler based on the AlarmType
     *
     * @param[in] alarmType = The type of the alarm
     */
    std::unique_ptr<AlarmHandler> obtainAlarmHandler(AlarmType alarmType);

    /**
     * @brief All alarms needs to be handled
     */
    std::map<AlarmKey, std::unique_ptr<sdeventplus::utility::Timer<
                           sdeventplus::ClockId::Monotonic>>>& alarms;

    /**
     * @brief The PowerState object
     */
    std::shared_ptr<phosphor::fan::PowerState> powerState;
};

} // namespace sensor::monitor
