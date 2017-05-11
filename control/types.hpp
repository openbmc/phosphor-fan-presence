#pragma once
#include <string>
#include <tuple>
#include <vector>

namespace phosphor
{
namespace fan
{
namespace control
{

class Zone;

//Placeholder. Conditions are completely TBD.
using Condition = bool;

constexpr auto fanNamePos = 0;
constexpr auto sensorListPos = 1;
using FanDefinition = std::tuple<std::string, std::vector<std::string>>;

constexpr auto intfPos = 0;
constexpr auto propPos = 1;
using Group = std::map<std::string, std::tuple<std::string, std::string>>;
using Handler = std::function<void(sdbusplus::bus::bus&,
                                   sdbusplus::message::message&,
                                   Zone&)>;
using Action = std::function<void(Zone&, const Group&)>;

constexpr auto signaturePos = 0;
constexpr auto handlerObjPos = 1;
using PropertyChange = std::tuple<std::string, Handler>;

constexpr auto groupPos = 0;
constexpr auto actionPos = 1;
constexpr auto propChangeListPos = 2;
using SetSpeedEvent = std::tuple<Group, Action, std::vector<PropertyChange>>;

constexpr auto eventGroupPos = 0;
constexpr auto eventHandlerPos = 1;
constexpr auto eventActionPos = 2;
using EventData = std::tuple<Group, Handler, Action>;

constexpr auto zoneObjPos = 0;
constexpr auto eventDataPos = 1;
using SignalEvent = std::tuple<Zone*, EventData>;

constexpr auto zoneNumPos = 0;
constexpr auto fullSpeedPos = 1;
constexpr auto fanListPos = 2;
constexpr auto setSpeedEventsPos = 3;
using ZoneDefinition = std::tuple<size_t,
                                  unsigned int,
                                  std::vector<FanDefinition>,
                                  std::vector<SetSpeedEvent>>;

constexpr auto conditionListPos = 0;
constexpr auto zoneListPos = 1;
using ZoneGroup = std::tuple<std::vector<Condition>,
                             std::vector<ZoneDefinition>>;

}
}
}
