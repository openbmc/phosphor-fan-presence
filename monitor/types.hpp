#pragma once

#include "trust_group.hpp"

#include <experimental/optional>
#include <functional>
#include <string>
#include <tuple>
#include <vector>

namespace phosphor
{
namespace fan
{
namespace monitor
{

constexpr auto propObj = 0;
constexpr auto propIface = 1;
constexpr auto propName = 2;
using PropertyIdentity = std::tuple<std::string, std::string, std::string>;

using PropertyValue = std::variant<bool, int64_t, std::string>;
constexpr auto propIdentity = 0;
constexpr auto propValue = 1;
using PropertyState = std::pair<PropertyIdentity, PropertyValue>;

using Condition = std::function<bool(sdbusplus::bus::bus&)>;

using CreateGroupFunction = std::function<std::unique_ptr<trust::Group>()>;

constexpr auto sensorNameField = 0;
constexpr auto hasTargetField = 1;
constexpr auto targetInterfaceField = 2;
constexpr auto factorField = 3;
constexpr auto offsetField = 4;

using SensorDefinition =
    std::tuple<std::string, bool, std::string, double, int64_t>;

constexpr auto fanNameField = 0;
constexpr auto funcDelay = 1;
constexpr auto timeoutField = 2;
constexpr auto fanDeviationField = 3;
constexpr auto numSensorFailsForNonfuncField = 4;
constexpr auto sensorListField = 5;
constexpr auto conditionField = 6;

using FanDefinition = std::tuple<std::string, size_t, size_t, size_t, size_t,
                                 std::vector<SensorDefinition>,
                                 std::experimental::optional<Condition>>;

} // namespace monitor
} // namespace fan
} // namespace phosphor
