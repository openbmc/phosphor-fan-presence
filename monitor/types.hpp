#pragma once

#include <string>
#include <tuple>
#include <vector>

namespace phosphor
{
namespace fan
{
namespace monitor
{

constexpr auto sensorNameField = 0;
constexpr auto hasTargetField = 1;

using SensorDefinition = std::tuple<std::string, bool>;

constexpr auto fanNameField = 0;
constexpr auto startupTimeoutField = 1;
constexpr auto timeoutField = 2;
constexpr auto fanDeviationField = 3;
constexpr auto numSensorFailsForNonfuncField = 4;
constexpr auto sensorListField = 5;

using FanDefinition = std::tuple<std::string,
                                 size_t,
                                 size_t,
                                 float,
                                 size_t,
                                 std::vector<SensorDefinition>>;

}
}
}
