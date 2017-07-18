#pragma once
#include <string>
#include <tuple>
#include <vector>
#include "utility.hpp"

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
// conditions for zones
using Condition = std::tuple<std::string,
                             std::vector<ConditionProperty>>;
// conditions for events/actions
using EventCondition = std::tuple<std::string,
                                  std::vector<ConditionProperty>>;
// ‘const class std::tuple<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, 
//                         std::vector<
//                                     std::tuple<
//                                                std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, 
//                                                std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, 
//                                                std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, 
//                                                bool
//                                               >, 
//                                     std::allocator<std::tuple<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, 
//                                                               std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, 
//                                                               std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, 
//                                                               bool
//                                                              > 
//                                                   > 
//                                    > 
//                        >’

using PropertyVariantType = sdbusplus::message::variant<bool, int64_t>;

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
constexpr auto eventConditionPos = 1;
constexpr auto actionPos = 2;
constexpr auto propChangeListPos = 3;
//using SetSpeedEvent = std::tuple<Group, Action, std::vector<PropertyChange>>;
using SetSpeedEvent = std::tuple<Group, EventCondition, Action, std::vector<PropertyChange>>;

constexpr auto eventGroupPos = 0;
constexpr auto eventHandlerPos = 1;
constexpr auto eventActionPos = 2;
using EventData = std::tuple<Group, Handler, Action>;

constexpr auto zoneNumPos = 0;
constexpr auto fullSpeedPos = 1;
constexpr auto floorSpeedPos = 2;
constexpr auto incDelayPos = 3;
constexpr auto decIntervalPos = 4;
constexpr auto fanListPos = 5;
constexpr auto setSpeedEventsPos = 6;
using ZoneDefinition = std::tuple<size_t,
                                  uint64_t,
                                  uint64_t,
                                  size_t,
                                  size_t,
                                  std::vector<FanDefinition>,
                                  std::vector<SetSpeedEvent>>;

constexpr auto conditionListPos = 0;
constexpr auto zoneListPos = 1;
using ZoneGroup = std::tuple<std::vector<Condition>,
                             std::vector<ZoneDefinition>>;

}
}
}
