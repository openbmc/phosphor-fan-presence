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
    /**
     * @brief Constructor
     *
     * @param[in] alarms - All alarms needs to be handled
     * @param[in] powerState - The PowerState object
     * @param[in] bus - The sdbusplus bus object
     * @param[in] event - The sdeventplus event object
     */
    AlarmChecker(
        std::map<AlarmKey, std::unique_ptr<sdeventplus::utility::Timer<
                               sdeventplus::ClockId::Monotonic>>>& alarms,
        std::shared_ptr<phosphor::fan::PowerState> powerState,
        sdbusplus::bus_t& bus, sdeventplus::Event& event);
    /**
     * @brief Check an alarm of the sensor
     *
     * @param[in] alarmType = The type of the alarm
     * @param[in] sensorPath = The sensor's path on dbus
     * @param[in] properties = The changed properties of the alarm interface
     */
    void checkAlarm(AlarmType alarmType, const std::string& sensorPath,
                    std::map<std::string, std::variant<bool>> properties);

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
    std::shared_ptr<AlarmHandler> obtainAlarmHandler(AlarmType alarmType);

    /**
     * @brief All alarms needs to be handled
     */
    std::map<AlarmKey, std::unique_ptr<sdeventplus::utility::Timer<
                           sdeventplus::ClockId::Monotonic>>>& alarms;

    /**
     * @brief The PowerState object
     */
    std::shared_ptr<phosphor::fan::PowerState> _powerState;

    /**
     * @brief The sdbusplus bus object
     */
    sdbusplus::bus_t& bus;

    /**
     * @brief The sdeventplus event object
     */
    sdeventplus::Event& event;
};

} // namespace sensor::monitor
