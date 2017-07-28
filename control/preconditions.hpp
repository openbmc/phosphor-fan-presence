#pragma once

namespace phosphor
{
namespace fan
{
namespace control
{
namespace precondition
{

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
auto property_states_match(std::vector<PrecondGroup>&& pg,
                           SetSpeedEvent&& sse)
{
    return [pg = std::move(pg),
            sse = std::move(sse)](auto& zone, auto& group)
    {
        // TODO Read/Compare given precondition entries
        // TODO Only init the event when the precondition(s) are true
        // TODO Remove the event properties when the precondition(s) are false
    };
}

} // namespace precondition
} // namespace control
} // namespace fan
} // namespace phosphor
