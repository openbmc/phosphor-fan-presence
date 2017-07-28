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
 * @brief A precondition to get a property's value and subscribe to its
 * property change signal
 * @details Initializes a precondition based on a dbus property's value where
 * it is cached within the zone and is read/updated along with subscribing to
 * it's property changed signal
 *
 * @param[in] pg - Precondition property group for property values
 * @param[in] sse - Set speed event definition
 *
 * @return Lambda function
 *     A lambda function to read and compare precondition property values and
 *     either subscribe or unsubscribe a set speed event group.
 */
auto propertyChanged(std::vector<PrecondGroup>&& pg, SetSpeedEvent&& sse)
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
