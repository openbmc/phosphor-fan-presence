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
 * @brief An action that wraps a list of actions with a timer
 * @details Sets up a list of actions to be invoked when the defined timer
 * expires (or for each expiration of a repeating timer).
 *
 * @param[in] tConf - Timer configuration parameters
 * @param[in] action - List of actions to be called when the timer expires
 *
 * @return Action lambda function
 *     An Action function that creates a timer
 */
Action call_actions_based_on_timer(
        TimerConf&& tConf,
        std::vector<Action>&& actions);

/**
 * @brief An action that sets the floor to the default fan floor speed
 * @details Sets the fan floor to the defined default fan floor speed when a
 * service associated to the given group has terminated. Once all services
 * are functional and providing the sensors again, the fan floor is allowed
 * to be set normally.
 *
 * @param[in] zone - Zone containing fans
 * @param[in] group - Group of sensors to determine services' states
 */
void default_floor_on_missing_owner(Zone& zone, const Group& group);

/**
 * @brief An action to set a speed when a service owner is missing
 * @details Sets the fans to the given speed when any service owner associated
 * to the group is missing. Once all services are functional and providing
 * the event data again, active fan speed changes are allowed.
 *
 * @param[in] speed - Speed to set the zone to
 *
 * @return Action lambda function
 *     An Action function that sets the zone to the given speed if any service
 *     owners are missing.
 */
Action set_speed_on_missing_owner(uint64_t speed);

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
        size_t numAtState = 0;
        for (auto& entry : group)
        {
            try
            {
                if (zone.template getPropertyValue<T>(
                        std::get<pathPos>(entry),
                        std::get<intfPos>(entry),
                        std::get<propPos>(entry)) == state)
                {
                    numAtState++;
                }
            }
            catch (const std::out_of_range& oore)
            {
                // Default to property not equal when not found
            }
            if (numAtState >= count)
            {
                zone.setSpeed(speed);
                break;
            }
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
                            std::get<pathPos>(entry),
                            std::get<intfPos>(entry),
                            std::get<propPos>(entry));
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
        for (auto& entry : group)
        {
            try
            {
                T value = zone.template getPropertyValue<T>(
                        std::get<pathPos>(entry),
                        std::get<intfPos>(entry),
                        std::get<propPos>(entry));
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
                else
                {
                    // No decrease allowed for this group
                    netDelta = 0;
                    break;
                }
            }
            catch (const std::out_of_range& oore)
            {
                // Property value not found, netDelta unchanged
            }
        }
        // Update group's decrease allowed state
        zone.setDecreaseAllow(&group, !(netDelta == 0));
        // Request speed decrease to occur on decrease interval
        zone.requestSpeedDecrease(netDelta);
    };
}

/**
 * @brief An action to use an alternate set of events
 * @details Provides the ability to replace a default set of events with an
 * alternate set of events based on all members of a group being at a specified
 * state. When any member of the group no longer matches the provided state,
 * the alternate set of events are replaced with the defaults.
 *
 * @param[in] state - State to compare the group's property value to
 * @param[in] defEvents - The default set of events
 * @param[in] altEvents - The alternate set of events
 *
 * @return Lambda function
 *     A lambda function that checks all group members are at a specified state
 * and replacing the default set of events with an alternate set of events.
 */
template <typename T>
auto use_alternate_events_on_state(T&& state,
                                   std::vector<SetSpeedEvent>&& defEvents,
                                   std::vector<SetSpeedEvent>&& altEvents)
{
    return [state = std::forward<T>(state),
            defEvents = std::move(defEvents),
            altEvents = std::move(altEvents)](auto& zone, auto& group)
    {
        // Compare all group entries to the state
        auto useAlt = std::all_of(
            group.begin(),
            group.end(),
            [&zone, &state](auto const& entry)
            {
                try
                {
                    return zone.template getPropertyValue<T>(
                            std::get<pathPos>(entry),
                            std::get<intfPos>(entry),
                            std::get<propPos>(entry)) == state;
                }
                catch (const std::out_of_range& oore)
                {
                    // Default to property not equal when not found
                    return false;
                }
            });

        const std::vector<SetSpeedEvent> *rmEvents = &altEvents;
        const std::vector<SetSpeedEvent> *initEvents = &defEvents;

        if (useAlt)
        {
            rmEvents = &defEvents;
            initEvents = &altEvents;
        }

        // Remove events
        std::for_each(
            rmEvents->begin(),
            rmEvents->end(),
            [&zone](auto const& entry)
            {
                zone.removeEvent(entry);
            });
        // Init events
        std::for_each(
            initEvents->begin(),
            initEvents->end(),
            [&zone](auto const& entry)
            {
                zone.initEvent(entry);
            });
    };
}

/**
 * @brief An action to set the floor speed on a zone
 * @details Using sensor group values that are within a defined range, the
 * floor speed is selected from the first map key entry that the median
 * sensor value is less than where 3 or more sensor group values are valid.
 * In the case where less than 3 sensor values are valid, use the highest
 * sensor group value and default the floor speed when 0 sensor group values
 * are valid.
 *
 * @param[in] lowerBound - Lowest allowed sensor value to be valid
 * @param[in] upperBound - Highest allowed sensor value to be valid
 * @param[in] valueToSpeed - Ordered map of sensor value-to-speed
 *
 * @return Action lambda function
 *     An Action function to set the zone's floor speed from a resulting group
 * of valid sensor values based on their highest value or median.
 */
Action set_floor_from_median_sensor_value(
        int64_t lowerBound,
        int64_t upperBound,
        std::map<int64_t, uint64_t>&& valueToSpeed);

} // namespace action
} // namespace control
} // namespace fan
} // namespace phosphor
