#pragma once

#include <algorithm>

namespace phosphor
{
namespace fan
{
namespace control
{
namespace action
{

/**
 * @brief An action to set the speed on a zone
 * @details The zone is set to the given speed when a defined number of
 * properties in the group are set to the given state
 *
 * @param[in] count - Number of properties
 * @param[in] state - Value the property(s) needed to be set at
 * @param[in] speed - Speed to set the zone to
 *
 * @return Lambda function
 *     A lambda function to set the zone speed when the number of properties
 *     within the group are at a certain value
 */
auto count_state_before_speed(size_t count, bool state, uint64_t speed)
{
    return [=](auto& zone, auto& group)
    {
        size_t numAtState = std::count_if(
            group.begin(),
            group.end(),
            [&zone, &state](auto const& entry)
            {
                return zone.getPropertyValue(entry.first,
                                             entry.second) == state;
            });
        if (numAtState >= count)
        {
            zone.setSpeed(speed);
        }
    };
}

} // namespace action
} // namespace control
} // namespace fan
} // namespace phosphor
