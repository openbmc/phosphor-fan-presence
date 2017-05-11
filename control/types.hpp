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

using Handler = std::function<void(sdbusplus::bus::bus&,
                                   sdbusplus::message::message&,
                                   Zone&)>;

constexpr auto signaturePos = 0;
constexpr auto handlerObjPos = 1;
using PropertyChange = std::tuple<std::string, Handler>;
using SetSpeedEvent = std::vector<PropertyChange>;

constexpr auto zoneObjPos = 0;
using SignalEvent = std::tuple<Zone*, Handler>;

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
