#pragma once

#include <algorithm>
#include <numeric>
#include "types.hpp"
#include "zone.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{
namespace action
{

/**
 * @brief An action to set the request speed base
 * @details A new target speed is determined using a speed delta being added
 * or subtracted, for increases or decrease respectively, from a base speed.
 * This base speed defaults to be the current target speed or is set to a
 * different base speed(i.e. the fans' tach feedback speed) to request a new
 * target from.
 *
 * @param[in] zone - Zone containing fans
 * @param[in] group - Group of sensors to determine base from
 */
void set_request_speed_base_with_max(Zone& zone, const Group& group);

/**
 * @brief An action to set the speed on a zone
 * @details The zone is held at the given speed when a defined number of
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
template <typename T>
auto count_state_before_speed(size_t count, T&& state, uint64_t speed)
{
    return [count,
            speed,
            state = std::forward<T>(state)](auto& zone, auto& group)
    {
        size_t numAtState = std::count_if(
            group.begin(),
            group.end(),
            [&zone, &state](auto const& entry)
            {
                try
                {
                    return zone.template getPropertyValue<T>(
                            entry.first,
                            std::get<intfPos>(entry.second),
                            std::get<propPos>(entry.second)) == state;
                }
                catch (const std::out_of_range& oore)
                {
                    // Default to property not equal when not found
                    return false;
                }
            });
        if (numAtState >= count)
        {
            zone.setSpeed(speed);
        }
        // Update group's fan control active allowed based on action results
        zone.setActiveAllow(&group, !(numAtState >= count));
    };
}

/**
 * @brief An action to set the floor speed on a zone
 * @details Based on the average of the defined sensor group values, the floor
 * speed is selected from the first map key entry that the average sensor value
 * is less than.
 *
 * @param[in] val_to_speed - Ordered map of sensor value-to-speed
 *
 * @return Action lambda function
 *     An Action function to set the zone's floor speed when the average of
 *     property values within the group is below the lowest sensor value given
 */
Action set_floor_from_average_sensor_value(
        std::map<int64_t, uint64_t>&& val_to_speed);

/**
 * @brief An action to set the ceiling speed on a zone
 * @details Based on the average of the defined sensor group values, the
 * ceiling speed is selected from the map key transition point that the average
 * sensor value falls within depending on the key values direction from what
 * was previously read.
 *
 * @param[in] val_to_speed - Ordered map of sensor value-to-speed transitions
 *
 * @return Action lambda function
 *     An Action function to set the zone's ceiling speed when the average of
 *     property values within the group is above(increasing) or
 *     below(decreasing) the key transition point
 */
Action set_ceiling_from_average_sensor_value(
        std::map<int64_t, uint64_t>&& val_to_speed);

/**
 * @brief An action to set the speed increase delta and request speed change
 * @details Provides the ability to determine what the net increase delta the
 * zone's fan speeds should be updated by from their current target speed and
 * request that new target speed.
 *
 * @param[in] state - State to compare the group's property value to
 * @param[in] factor - Factor to apply to the calculated net delta
 * @param[in] speedDelta - Speed delta of the group
 *
 * @return Lambda function
 *     A lambda function that determines the net increase delta and requests
 * a new target speed with that increase for the zone.
 */
template <typename T>
auto set_net_increase_speed(T&& state, T&& factor, uint64_t speedDelta)
{
    return [speedDelta,
            factor = std::forward<T>(factor),
            state = std::forward<T>(state)](auto& zone, auto& group)
    {
        auto netDelta = zone.getIncSpeedDelta();
        std::for_each(
            group.begin(),
            group.end(),
            [&zone, &state, &factor, &speedDelta, &netDelta](
                auto const& entry)
            {
                try
                {
                    T value = zone.template getPropertyValue<T>(
                            entry.first,
                            std::get<intfPos>(entry.second),
                            std::get<propPos>(entry.second));
                    // TODO openbmc/phosphor-fan-presence#7 - Support possible
                    // state types for comparison
                    if (value >= state)
                    {
                        // Increase by at least a single delta(factor)
                        // to attempt bringing under 'state'
                        auto delta = std::max(
                            (value - state),
                            factor);
                        // Increase is the factor applied to the
                        // difference times the given speed delta
                        netDelta = std::max(
                            netDelta,
                            (delta/factor) * speedDelta);
                    }
                }
                catch (const std::out_of_range& oore)
                {
                    // Property value not found, netDelta unchanged
                }
            }
        );
        // Request speed change for target speed update
        zone.requestSpeedIncrease(netDelta);
    };
}

/**
 * @brief An action to set the speed decrease delta and request speed change
 * @details Provides the ability to determine what the net decrease delta each
 * zone's fan speeds should be updated by from their current target speed, and
 * request that speed change occur on the next decrease interval.
 *
 * @param[in] state - State to compare the group's property value to
 * @param[in] factor - Factor to apply to the calculated net delta
 * @param[in] speedDelta - Speed delta of the group
 *
 * @return Lambda function
 *     A lambda function that determines the net decrease delta and requests
 * a new target speed with that decrease for the zone.
 */
template <typename T>
auto set_net_decrease_speed(T&& state, T&& factor, uint64_t speedDelta)
{
    return [speedDelta,
            factor = std::forward<T>(factor),
            state = std::forward<T>(state)](auto& zone, auto& group)
    {
        auto netDelta = zone.getDecSpeedDelta();
        std::for_each(
            group.begin(),
            group.end(),
            [&zone, &state, &factor, &speedDelta, &netDelta](auto const& entry)
            {
                try
                {
                    T value = zone.template getPropertyValue<T>(
                            entry.first,
                            std::get<intfPos>(entry.second),
                            std::get<propPos>(entry.second));
                    // TODO openbmc/phosphor-fan-presence#7 - Support possible
                    // state types for comparison
                    if (value < state)
                    {
                        if (netDelta == 0)
                        {
                            netDelta = ((state - value)/factor) * speedDelta;
                        }
                        else
                        {
                            // Decrease is the factor applied to the
                            // difference times the given speed delta
                            netDelta = std::min(
                                netDelta,
                                ((state - value)/factor) * speedDelta);
                        }
                    }
                }
                catch (const std::out_of_range& oore)
                {
                    // Property value not found, netDelta unchanged
                }
            }
        );
        // Request speed decrease to occur on decrease interval
        zone.requestSpeedDecrease(netDelta);
    };
}

} // namespace action
} // namespace control
} // namespace fan
} // namespace phosphor
