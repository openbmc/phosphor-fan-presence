#pragma once
#include "power_state.hpp"
#include "sdbusplus.hpp"
#include "types.hpp"

#include <nlohmann/json.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <chrono>
#include <map>
#include <memory>

namespace sensor::monitor
{

/**
 * @class AlarmHandler
 *
 * This abstract class defines the interface of AlarmHandler, and
 * store common objects.
 *
 */
class AlarmHandler
{
  public:
    /**
     * @brief Constructor
     *
     * @param[in] alarms - All alarms needs to be handled
     */
    AlarmHandler(
        std::map<AlarmKey, std::unique_ptr<sdeventplus::utility::Timer<
                               sdeventplus::ClockId::Monotonic>>>& alarms) :
        alarms(alarms)
    {}

    virtual ~AlarmHandler() = default;

    /**
     * @brief Interface for checking alarm, override it in the children classes
     */
    virtual void
        checkAlarm(std::string sensorPath,
                   std::map<std::string, std::variant<bool>> properties) = 0;

  protected:
    /**
     * @brief All alarms needs to be handled
     */
    std::map<AlarmKey, std::unique_ptr<sdeventplus::utility::Timer<
                           sdeventplus::ClockId::Monotonic>>>& alarms;
};

/**
 * @class ProtectionAlarmHandler
 *
 * This class checks the instances of the D-Bus interfaces
 * on specific sensor:
 *   xyz.openbmc_project.Sensor.Threshold.SoftShutdown
 *   xyz.openbmc_project.Sensor.Threshold.HardShutdown
 *
 * If they trip the code will start a timer, at the end of which the
 * system will be shut down and send SystemProtectionTriggered event.
 * The timer values can be modified with configure.ac options. If the
 * alarm is cleared before the timer expires, then the timer is stopped.
 *
 * Event logs are also created when the alarms trip and clear.
 *
 * Note that the SoftShutdown alarm code actually implements a hard shutdown.
 * This is because in the system this is being written for, the host is
 * driving the shutdown process (i.e. doing a soft shutdown) based on an alert
 * it receives via another channel.  If the soft shutdown timer expires, it
 * means that the host didn't do a soft shutdown in the time allowed and
 * now a hard shutdown is required.  This behavior could be modified with
 * compile flags if anyone needs a different behavior in the future.
 */
class ProtectionAlarmHandler : public AlarmHandler
{
  public:
    /**
     * @brief Constructor
     *
     * @param[in] alarms - All alarms needs to be handled
     * @param[in] type - The handled AlarmType
     * @param[in] powerState - The PowerState object
     */
    ProtectionAlarmHandler(
        std::map<AlarmKey, std::unique_ptr<sdeventplus::utility::Timer<
                               sdeventplus::ClockId::Monotonic>>>& alarms,
        AlarmType type, std::shared_ptr<phosphor::fan::PowerState> powerState) :
        AlarmHandler(alarms),
        handledType(type), powerState(powerState)
    {}

    /**
     * @brief Checks HardShutdown and SoftShutdown alarm on the sensor
     *        Skip if the current powerState is off
     *
     * @param[in] sensorPath = The sensor's path on dbus
     * @param[in] properties = The changed properties of the alarm interface
     */
    void checkAlarm(
        std::string sensorPath,
        std::map<std::string, std::variant<bool>> properties) override;

  private:
    /**
     * @brief Checks if the sensor is back to normal
     */
    bool isBackToNormal(bool isAlarmTriggered, bool hasTimerBeenStarted);

    /**
     * @brief Checks an alarm value to see if a shutdown timer needs
     *        to be started or stopped.
     *
     * If the alarm is triggered and the timer isn't running, start it.
     * If the alarm is not triggered and the timer is running, stop it.
     *
     * @param[in] isAlarmTriggered - The alarm property value
     * @param[in] alarmKey - The alarm key
     */
    void check(bool isAlarmTriggered, const AlarmKey& alarmKey);

    /**
     * @brief Create a BMC Dump
     */
    void createBmcDump() const;

    /**
     * @brief Triggers system protection.
     *
     * The function called when the timer expires.
     *
     * @param[in] alarmKey - The alarm key
     */
    void triggerProtection(const AlarmKey& alarmKey);

    /**
     * @brief Starts a shutdown timer.
     *
     * @param[in] alarmKey - The alarm key
     */
    void startTimer(const AlarmKey& alarmKey);

    /**
     * @brief Stops a shutdown timer.
     *
     * @param[in] alarmKey - The alarm key
     */
    void stopTimer(const AlarmKey& alarmKey);

    /**
     * @brief Creates a phosphor-logging event log
     *
     * @param[in] alarmKey - The alarm key
     * @param[in] alarmValue - The alarm property value
     * @param[in] sensorValue - The sensor value behind the alarm.
     * @param[in] isPowerOffError - If this is committed at the time of the
     *                              power off.
     */
    void createEventLog(const AlarmKey& alarmKey, bool alarmValue,
                        const std::optional<double>& sensorValue,
                        bool isPowerOffError = false);

    /**
     * @brief The handled AlarmType
     */
    AlarmType handledType;

    /**
     * @brief The PowerState object
     */
    std::shared_ptr<phosphor::fan::PowerState> powerState;
};

/**
 * @class RecoveryAlarmHandler
 *
 * This class checks if the system needs to be recovered after protection
 * is triggered.
 *
 * After a system protection is triggered and the recovery action of the
 * sensor is defined in the config file (i.e., recovery-action.json), system
 * recovery detection will be switched on, start listening the alarm properties
 * defined in the config.
 *
 * If they trip the code will start a timer, at the end of which the
 * system will be recovery and send SystemRecoveryTriggered event.
 * The timer values can be modified with recovery-action.json. If the
 * alarm is alerted before the timer expires, then the timer is stopped.
 * The service of the system recovery can also been defined in the config.
 */
class RecoveryAlarmHandler : public AlarmHandler
{
  public:
    /**
     * @brief Constructor
     *
     * @param[in] alarms - All alarms needs to be handled
     * @param[in] type - The handled AlarmType
     */
    RecoveryAlarmHandler(
        std::map<AlarmKey, std::unique_ptr<sdeventplus::utility::Timer<
                               sdeventplus::ClockId::Monotonic>>>& alarms,
        AlarmType type) :
        AlarmHandler(alarms),
        handledType(type)
    {}

    /**
     * @brief Checks the alarms on the sensor. Skip if there is no alarm
     *        defined in recovery-action.json
     *
     * @param[in] sensorPath = The sensor's path on dbus
     * @param[in] properties = The changed properties of the alarm interface
     */
    void checkAlarm(
        std::string sensorPath,
        std::map<std::string, std::variant<bool>> properties) override;

  private:
    /**
     * @brief Checks if the sensor is back to abnormal
     */
    bool isBackToAbnormal(bool isAlarmTriggered, bool hasTimerBeenStarted);

    /**
     * @brief Checks an alarm value to see if a recovery timer needs
     *        to be started or stopped.
     *
     * If the alarm is not triggered and the timer isn't running, start it.
     * If the alarm is triggered and the timer is running, stop it.
     *
     *
     * @param[in] isAlarmTriggered - The alarm property value
     * @param[in] alarmKey - The alarm key
     * @param[in] timerDelay - The timer delay defined in the config file
     */
    void check(bool isAlarmTriggered, const AlarmKey& alarmKey,
               uint16_t timerDelay);

    /**
     * @brief Triggers system recovery.
     *
     * The function called when the timer expires.
     *
     * @param[in] alarmKey - The alarm key
     */
    void triggerRecovery(const AlarmKey& alarmKey);

    /**
     * Load the configuration
     */
    nlohmann::json loadRecoveryActionConfig();

    /**
     * Obtains timer config for the sensor
     */
    std::map<AlarmDirection, uint16_t>
        obtainTimerConfigFor(const std::string& sensorPath);

    /**
     * @brief Starts a recovery timer.
     *
     * @param[in] alarmKey - The alarm key
     * @param[in] timerDelay - The timer delay defined in the config file
     */
    void startTimer(const AlarmKey& alarmKey, uint16_t timerDelay);

    /**
     * @brief Stops a recovery timer.
     *
     * @param[in] alarmKey - The alarm key
     */
    void stopTimer(const AlarmKey& alarmKey);

    /**
     * The file name of the recovery action configuration
     */
    const std::string recoveryConfigName = "recovery-action.json";

    /**
     * @brief The handled AlarmType
     */
    AlarmType handledType;
};

} // namespace sensor::monitor
