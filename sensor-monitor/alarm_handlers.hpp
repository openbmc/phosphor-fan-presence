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

class AlarmHandler
{
  public:
    AlarmHandler(
        std::map<AlarmKey, std::unique_ptr<sdeventplus::utility::Timer<
                               sdeventplus::ClockId::Monotonic>>>& alarms,
        sdbusplus::bus_t& bus) :
        _alarms(alarms),
        _bus(bus)
    {}
    virtual ~AlarmHandler() = default;
    virtual void
        checkAlarm(std::string sensorPath,
                   std::map<std::string, std::variant<bool>> properties) = 0;

  protected:
    std::map<AlarmKey, std::unique_ptr<sdeventplus::utility::Timer<
                           sdeventplus::ClockId::Monotonic>>>& _alarms;
    sdbusplus::bus_t& _bus;
};

class ProtectionAlarmHandler : public AlarmHandler
{
  public:
    ProtectionAlarmHandler(
        std::map<AlarmKey, std::unique_ptr<sdeventplus::utility::Timer<
                               sdeventplus::ClockId::Monotonic>>>& alarms,
        sdbusplus::bus_t& bus, sdeventplus::Event& event, AlarmType type,
        std::shared_ptr<phosphor::fan::PowerState> powerState) :
        AlarmHandler(alarms, bus),
        _event(event), _handledType(type), _powerState(powerState)
    {}
    void checkAlarm(
        std::string sensorPath,
        std::map<std::string, std::variant<bool>> properties) override;

  private:
    bool isBackToNormal(const bool hasTimerBeenStarted,
                        const bool isAlarmTriggered);
    void check(bool isAlarmTriggered, const AlarmKey& alarmKey);
    void createBmcDump() const;
    void triggerProtection(const AlarmKey& alarmKey);
    void startTimer(const AlarmKey& alarmKey);
    void stopTimer(const AlarmKey& alarmKey);
    void createEventLog(const AlarmKey& alarmKey, bool alarmValue,
                        const std::optional<double>& sensorValue,
                        bool isPowerOffError = false);
    sdeventplus::Event& _event;
    AlarmType _handledType;
    std::shared_ptr<phosphor::fan::PowerState> _powerState;
};

class RecoveryAlarmHandler : public AlarmHandler
{
  public:
    RecoveryAlarmHandler(
        std::map<AlarmKey, std::unique_ptr<sdeventplus::utility::Timer<
                               sdeventplus::ClockId::Monotonic>>>& alarms,
        sdbusplus::bus_t& bus, sdeventplus::Event& event, AlarmType type);
    void checkAlarm(
        std::string sensorPath,
        std::map<std::string, std::variant<bool>> properties) override;

  private:
    bool isBackToAbnormal(const bool hasTimerBeenStarted,
                          const bool isAlarmTriggered);
    void check(bool isAlarmTriggered, const AlarmKey& alarmKey,
               uint16_t timerDelay);
    void triggerRecovery(const AlarmKey& alarmKey);
    nlohmann::json loadRecoveryActionConfig();
    std::map<AlarmDirection, uint16_t>
        obtainTimerConfigFor(const std::string& sensorPath);
    void startTimer(const AlarmKey& alarmKey, uint16_t timerDelay);
    void stopTimer(const AlarmKey& alarmKey);
    const std::string recoveryConfigName = "recovery-action.json";
    sdeventplus::Event& _event;
    AlarmType _handledType;
};

} // namespace sensor::monitor
