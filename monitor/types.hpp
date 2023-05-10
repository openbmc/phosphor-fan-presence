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

struct SensorDefinition
{
    std::string name;
    bool hasTarget;
    std::string targetInterface;
    std::string targetPath;
    double factor;
    int64_t offset;
    size_t threshold;
    bool ignoreAboveMax;
};

struct FanDefinition
{
    std::string name;
    size_t method;
    size_t funcDelay;
    size_t timeout;
    size_t deviation;
    size_t upperDeviation;
    size_t numSensorFailsForNonfunc;
    size_t monitorStartDelay;
    size_t countInterval;
    std::optional<size_t> nonfuncRotorErrDelay;
    std::optional<size_t> fanMissingErrDelay;
    std::vector<SensorDefinition> sensorList;
    std::optional<Condition> condition;
    bool funcOnPresent;
};

constexpr auto presentHealthPos = 0;
constexpr auto sensorFuncHealthPos = 1;

using FanHealthEntry = std::tuple<bool, std::vector<bool>>;
using FanHealth = std::map<std::string, FanHealthEntry>;
} // namespace monitor
} // namespace fan
} // namespace phosphor
