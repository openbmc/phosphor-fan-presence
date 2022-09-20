#pragma once

#include "tach_sensor.hpp"
#include "trust_group.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Object/Enable/server.hpp>

#include <functional>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace phosphor
{
namespace fan
{
namespace monitor
{

template <typename... T>
using ServerObject = typename sdbusplus::server::object_t<T...>;

using ObjectEnableInterface =
    sdbusplus::xyz::openbmc_project::Object::server::Enable;

using ThermalAlertObject = ServerObject<ObjectEnableInterface>;

constexpr auto propObj = 0;
constexpr auto propIface = 1;
constexpr auto propName = 2;
using PropertyIdentity = std::tuple<std::string, std::string, std::string>;

using PropertyValue = std::variant<bool, int64_t, std::string>;
class JsonTypeHandler
{
    using json = nlohmann::json;

  public:
    /**
     * @brief Determines the data type of a JSON configured parameter that is
     * used as a variant within the fan monitor application and returns the
     * value as that variant.
     * @details Retrieves a JSON entry by the first derived data type that
     * is not null. Expected data types should appear in a logical order of
     * conversion. i.e.) uint and int could both be uint Alternatively, the
     * expected data type can be given to force which supported data type
     * the JSON entry should be retrieved as.
     *
     * @param[in] entry - A single JSON entry
     * @param[in] type - (OPTIONAL) The preferred data type of the entry
     *
     * @return A `PropertyValue` variant containing the JSON entry's value
     */
    static const PropertyValue getPropValue(const json& entry,
                                            const std::string& type = "")
    {
        PropertyValue value;
        if (auto boolPtr = entry.get_ptr<const bool*>())
        {
            if (type.empty() || type == "bool")
            {
                value = *boolPtr;
                return value;
            }
        }
        if (auto int64Ptr = entry.get_ptr<const int64_t*>())
        {
            if (type.empty() || type == "int64_t")
            {
                value = *int64Ptr;
                return value;
            }
        }
        if (auto stringPtr = entry.get_ptr<const std::string*>())
        {
            if (type.empty() || type == "std::string")
            {
                value = *stringPtr;
                return value;
            }
        }

        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Unsupported data type for JSON entry's value",
            phosphor::logging::entry("GIVEN_ENTRY_TYPE=%s", type.c_str()),
            phosphor::logging::entry("JSON_ENTRY=%s", entry.dump().c_str()),
            phosphor::logging::entry("SUPPORTED_TYPES=%s",
                                     "{bool, int64_t, std::string}"));
        throw std::runtime_error(
            "Unsupported data type for JSON entry's value");
    }
};

constexpr auto propIdentity = 0;
constexpr auto propValue = 1;
using PropertyState = std::pair<PropertyIdentity, PropertyValue>;

using Condition = std::function<bool(sdbusplus::bus_t&)>;

using CreateGroupFunction = std::function<std::unique_ptr<trust::Group>()>;

constexpr auto sensorNameField = 0;
constexpr auto hasTargetField = 1;
constexpr auto targetInterfaceField = 2;
constexpr auto targetPathField = 3;
constexpr auto factorField = 4;
constexpr auto offsetField = 5;
constexpr auto thresholdField = 6;
constexpr auto ignoreAboveMaxField = 7;

using SensorDefinition = std::tuple<std::string, bool, std::string, std::string,
                                    double, int64_t, size_t, bool>;

constexpr auto fanNameField = 0;
constexpr auto methodField = 1;
constexpr auto funcDelay = 2;
constexpr auto timeoutField = 3;
constexpr auto fanDeviationField = 4;
constexpr auto numSensorFailsForNonfuncField = 5;
constexpr auto monitorStartDelayField = 6;
constexpr auto countIntervalField = 7;
constexpr auto nonfuncRotorErrDelayField = 8;
constexpr auto fanMissingErrDelayField = 9;
constexpr auto sensorListField = 10;
constexpr auto conditionField = 11;
constexpr auto funcOnPresentField = 12;

using FanDefinition =
    std::tuple<std::string, size_t, size_t, size_t, size_t, size_t, size_t,
               size_t, std::optional<size_t>, std::optional<size_t>,
               std::vector<SensorDefinition>, std::optional<Condition>, bool>;

constexpr auto presentHealthPos = 0;
constexpr auto sensorFuncHealthPos = 1;

using FanHealthEntry = std::tuple<bool, std::vector<bool>>;
using FanHealth = std::map<std::string, FanHealthEntry>;
} // namespace monitor
} // namespace fan
} // namespace phosphor
