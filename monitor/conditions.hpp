#pragma once

#include "types.hpp"

namespace phosphor
{
namespace fan
{
namespace monitor
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

namespace condition
{

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
Condition propertiesMatch(std::vector<PropertyState>&& propStates);

} // namespace condition
} // namespace monitor
} // namespace fan
} // namespace phosphor
