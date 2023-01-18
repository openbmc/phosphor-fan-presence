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

class AlarmChecker
{
  public:
    AlarmChecker(
        std::map<AlarmKey, std::unique_ptr<sdeventplus::utility::Timer<
                               sdeventplus::ClockId::Monotonic>>>& alarms,
        std::shared_ptr<phosphor::fan::PowerState> powerState,
        sdbusplus::bus_t& bus, sdeventplus::Event& event) :
        alarms(alarms),
        _powerState(powerState), bus(bus), event(event)
    {}
    void checkAlarm(AlarmType alarmType, const std::string& sensorPath,
                    std::map<std::string, std::variant<bool>> properties);
    void checkAlarms();

  private:
    std::shared_ptr<AlarmHandler> obtainAlarmHandler(AlarmType alarmType);
    std::map<AlarmKey, std::unique_ptr<sdeventplus::utility::Timer<
                           sdeventplus::ClockId::Monotonic>>>& alarms;
    std::shared_ptr<phosphor::fan::PowerState> _powerState;
    sdbusplus::bus_t& bus;
    sdeventplus::Event& event;
};

} // namespace sensor::monitor
