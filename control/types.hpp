#pragma once
#include <string>
#include <tuple>
#include <vector>
#include <sdbusplus/server.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/utility/timer.hpp>

namespace phosphor
{
namespace fan
{
namespace control
{

class Zone;

constexpr auto propertyNamePos = 0;
constexpr auto propertyInterfacePos = 1;
constexpr auto propertyPathPos = 2;
constexpr auto propertyValuePos = 3;

// TODO openbmc/openbmc#1769: Support more property types.
using ConditionProperty = std::tuple<std::string,
                          std::string,
                          std::string,
                          bool>;

constexpr auto conditionTypePos = 0;
constexpr auto conditionPropertyListPos = 1;
using Condition = std::tuple<std::string,
                             std::vector<ConditionProperty>>;

using PropertyVariantType = sdbusplus::message::variant<bool,
                                                        int64_t,
                                                        std::string>;

constexpr auto fanNamePos = 0;
constexpr auto sensorListPos = 1;
constexpr auto targetInterfacePos = 2;
using FanDefinition = std::tuple<std::string,
                                 std::vector<std::string>,
                                 std::string>;

constexpr auto intfPos = 0;
constexpr auto propPos = 1;
using Group = std::map<std::string, std::tuple<std::string, std::string>>;
using ZoneHandler = std::function<void(Zone&)>;
using Handler = std::function<void(sdbusplus::bus::bus&,
                                   sdbusplus::message::message&,
                                   Zone&)>;
using Action = std::function<void(Zone&, const Group&)>;
using Trigger = std::function<void(Zone&,
                                   const Group&,
                                   const std::vector<Action>&)>;

constexpr auto pcPathPos = 0;
constexpr auto pcIntfPos = 1;
constexpr auto pcPropPos = 2;
constexpr auto pcValuePos = 3;
using PrecondGroup = std::tuple<std::string,
                                std::string,
                                std::string,
                                PropertyVariantType>;

constexpr auto namePos = 0;
constexpr auto hasOwnerPos = 1;
using Service = std::tuple<std::string, bool>;

constexpr auto intervalPos = 0;
constexpr auto typePos = 1;
enum class TimerType
{
    oneshot,
    repeating,
};
using TimerConf = std::tuple<std::chrono::microseconds,
                             TimerType>;

constexpr auto sigMatchPos = 0;
constexpr auto sigHandlerPos = 1;
using Signal = std::tuple<std::string, Handler>;

constexpr auto groupPos = 0;
constexpr auto actionsPos = 1;
constexpr auto triggerPos = 2;
constexpr auto signalsPos = 3;
using SetSpeedEvent = std::tuple<Group,
                                 std::vector<Action>,
                                 std::vector<Trigger>,
                                 std::vector<Signal>>;

constexpr auto eventGroupPos = 0;
constexpr auto eventMatchPos = 1;
constexpr auto eventHandlerPos = 2;
constexpr auto eventActionsPos = 3;
using EventData = std::tuple<Group, std::string, Handler, std::vector<Action>>;

constexpr auto timerEventDataPos = 0;
constexpr auto timerTimerPos = 1;
using Timer = sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>;
using TimerEvent = std::tuple<std::unique_ptr<EventData>, Timer>;

constexpr auto signalEventDataPos = 0;
constexpr auto signalMatchPos = 1;
using SignalEvent =
    std::tuple<std::unique_ptr<EventData>,
               std::unique_ptr<sdbusplus::server::match::match>>;

constexpr auto zoneNumPos = 0;
constexpr auto fullSpeedPos = 1;
constexpr auto floorSpeedPos = 2;
constexpr auto incDelayPos = 3;
constexpr auto decIntervalPos = 4;
constexpr auto handlerPos = 5;
constexpr auto fanListPos = 6;
constexpr auto setSpeedEventsPos = 7;
using ZoneDefinition = std::tuple<size_t,
                                  uint64_t,
                                  uint64_t,
                                  size_t,
                                  size_t,
                                  std::vector<ZoneHandler>,
                                  std::vector<FanDefinition>,
                                  std::vector<SetSpeedEvent>>;

constexpr auto conditionListPos = 0;
constexpr auto zoneListPos = 1;
using ZoneGroup = std::tuple<std::vector<Condition>,
                             std::vector<ZoneDefinition>>;

}
}
}
