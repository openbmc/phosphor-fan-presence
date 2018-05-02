#pragma once

#include <algorithm>
#include "types.hpp"
#include "sdbusplus.hpp"

namespace phosphor
{
namespace fan
{
namespace monitor
{
namespace condition
{

/**
 * @brief Create a condition function object
 *
 * @param[in] condition - The condition being created
 *
 * @return - The created condition function object
 */
template <typename T>
auto make_condition(T&& condition)
{
    return Condition(std::forward<T>(condition));
}

/**
 * @brief A condition that checks all properties match the given values
 * @details Checks each property entry against its given value where all
 * property values must match their given value for the condition to pass
 *
 * @param[in] propStates - List of property identifiers and their value
 *
 * @return Condition lambda function
 *     A Condition function that checks all properties match
 */
auto propertiesMatch(std::vector<PropertyState>&& propStates)
{
    return [pStates = std::move(propStates)](sdbusplus::bus::bus& bus)
    {
        return std::all_of(
            pStates.begin(),
            pStates.end(),
            [&bus](const auto& p)
        {
            return util::SDBusPlus::getProperty<decltype(p.second)>(
                bus,
                std::get<propObj>(p.first),
                std::get<propIface>(p.first),
                std::get<propName>(p.first)) == p.second;
        });
    };
}

} // namespace condition
} // namespace monitor
} // namespace fan
} // namespace phosphor
