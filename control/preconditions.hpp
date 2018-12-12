#pragma once

#include "types.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{
namespace precondition
{

using namespace phosphor::logging;

/**
 * @brief A precondition to compare a group of property values and
 * subscribe/unsubscribe a set speed event group
 * @details Compares each entry within the precondition group to a given value
 * that when each entry's property value matches the given value, the set speed
 * event is then initialized. At any point a precondition entry's value no
 * longer matches, the set speed event is removed from being active and fans
 * are set to full speed.
 *
 * @param[in] pg - Precondition property group of property values
 * @param[in] sse - Set speed event definition
 *
 * @return Lambda function
 *     A lambda function to compare precondition property value states
 *     and either subscribe or unsubscribe a set speed event group.
 */
Action property_states_match(std::vector<PrecondGroup>&& pg,
                             std::vector<SetSpeedEvent>&& sse);

} // namespace precondition
} // namespace control
} // namespace fan
} // namespace phosphor
