#pragma once

#include <functional>
#include <string>
#include <tuple>
#include <vector>
#include "trust_group.hpp"

namespace phosphor
{
namespace fan
{
namespace monitor
{

using CreateGroupFunction =
        std::function<std::unique_ptr<trust::Group>()>;

constexpr auto sensorNameField = 0;
constexpr auto hasTargetField = 1;

using SensorDefinition = std::tuple<std::string, bool>;

constexpr auto fanNameField = 0;
constexpr auto timeoutField = 1;
constexpr auto fanDeviationField = 2;
constexpr auto numSensorFailsForNonfuncField = 3;
constexpr auto sensorListField = 4;

using FanDefinition = std::tuple<std::string,
                                 size_t,
                                 size_t,
                                 size_t,
                                 std::vector<SensorDefinition>>;

}
}
}
