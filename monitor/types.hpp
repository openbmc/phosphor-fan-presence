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
constexpr auto targetInterfaceField = 2;
constexpr auto factorField = 3;
constexpr auto offsetField = 4;

using SensorDefinition = std::tuple<std::string,
                                    bool,
                                    std::string,
                                    size_t,
                                    size_t>;

constexpr auto fanNameField = 0;
constexpr auto funcDelay = 1;
constexpr auto timeoutField = 2;
constexpr auto fanDeviationField = 3;
constexpr auto numSensorFailsForNonfuncField = 4;
constexpr auto sensorListField = 5;

using FanDefinition = std::tuple<std::string,
                                 size_t,
                                 size_t,
                                 size_t,
                                 size_t,
                                 std::vector<SensorDefinition>>;

}
}
}
