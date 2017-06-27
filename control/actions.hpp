#pragma once

#include <algorithm>
#include <numeric>

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
                return zone.template getPropertyValue<T>(
                        entry.first,
                        std::get<intfPos>(entry.second),
                        std::get<propPos>(entry.second)) == state;
            });
        // Update group's fan control active allowed based on action results
        zone.setActiveAllow(&group, !(numAtState >= count));
        if (numAtState >= count)
        {
            zone.setSpeed(speed);
        }
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
 * @return Lambda function
 *     A lambda function to set the zone's floor speed when the average of
 *     property values within the group is below the lowest sensor value given
 */
auto set_floor_from_average_sensor_value(
        std::map<int64_t, uint64_t>&& val_to_speed)
{
    return [val_to_speed = std::move(val_to_speed)](auto& zone, auto& group)
    {
        auto speed = zone.getDefFloor();
        if (group.size() != 0)
        {
            auto sumValue = std::accumulate(
                    group.begin(),
                    group.end(),
                    0,
                    [&zone](int64_t sum, auto const& entry)
                    {
                        return sum + zone.template getPropertyValue<int64_t>(
                                entry.first,
                                std::get<intfPos>(entry.second),
                                std::get<propPos>(entry.second));
                    });
            auto avgValue= sumValue / group.size();
            auto it = std::find_if(
                val_to_speed.begin(),
                val_to_speed.end(),
                [&avgValue](auto const& entry)
                {
                    return avgValue < entry.first;
                }
            );
            if (it != std::end(val_to_speed))
            {
                speed = (*it).second;
            }
        }
        zone.setFloor(speed);
    };
}

/**
 * @brief An action to set the ceiling speed on a zone
 * @details Based on the average of the defined sensor group values, the ceiling
 * speed is selected from the map key transition point that the average sensor
 * value falls within depending on the key values direction from what was
 * previously read.
 *
 * @param[in] val_to_speed - Ordered map of sensor value-to-speed transitions
 *
 * @return Lambda function
 *     A lambda function to set the zone's ceiling speed when the average of
 *     property values within the group is above(increasing) or
 *     below(decreasing) the key transition point
 */
auto set_ceiling_from_average_sensor_value(
        std::map<int64_t, uint64_t>&& val_to_speed)
{
    return [val_to_speed = std::move(val_to_speed)](auto& zone, auto& group)
    {
        auto speed = zone.getCeiling();
        if (group.size() != 0)
        {
            auto sumValue = std::accumulate(
                    group.begin(),
                    group.end(),
                    0,
                    [&zone](int64_t sum, auto const& entry)
                    {
                        return sum + zone.template getPropertyValue<int64_t>(
                                entry.first,
                                std::get<intfPos>(entry.second),
                                std::get<propPos>(entry.second));
                    });
            auto avgValue = sumValue / group.size();
            auto prevValue = zone.swapCeilingKeyValue(avgValue);
            if (avgValue != prevValue)
            {// Only check if previous and new values differ
                if (avgValue < prevValue)
                {// Value is decreasing from previous
                    for (auto it = val_to_speed.rbegin();
                         it != val_to_speed.rend();
                         ++it)
                    {
                        if (it == val_to_speed.rbegin() &&
                            avgValue >= it->first)
                        {
                            // Value is at/above last map key,
                            // set ceiling speed to the last map key's value
                            speed = it->second;
                            break;
                        }
                        else if (std::next(it, 1) == val_to_speed.rend() &&
                                 avgValue <= it->first)
                        {
                            // Value is at/below first map key,
                            // set ceiling speed to the first map key's value
                            speed = it->second;
                            break;
                        }
                        if (avgValue < it->first &&
                            it->first <= prevValue)
                        {
                            // Value decreased & transitioned across a map key,
                            // update ceiling speed to this map key's value
                            // when new value is below map's key and the key
                            // is at/below the previous value
                            speed = it->second;
                        }
                    }
                }
                else
                {// Value is increasing from previous
                    for (auto it = val_to_speed.begin();
                         it != val_to_speed.end();
                         ++it)
                    {
                        if (it == val_to_speed.begin() &&
                            avgValue <= it->first)
                        {
                            // Value is at/below first map key,
                            // set ceiling speed to the first map key's value
                            speed = it->second;
                            break;
                        }
                        else if (std::next(it, 1) == val_to_speed.end() &&
                                 avgValue >= it->first)
                        {
                            // Value is at/above last map key,
                            // set ceiling speed to the last map key's value
                            speed = it->second;
                            break;
                        }
                        if (avgValue > it->first &&
                            it->first >= prevValue)
                        {
                            // Value increased & transitioned across a map key,
                            // update ceiling speed to this map key's value
                            // when new value is above map's key and the key
                            // is at/above the previous value
                            speed = it->second;
                        }
                    }
                }
            }
        }
        zone.setCeiling(speed);
    };
}

} // namespace action
} // namespace control
} // namespace fan
} // namespace phosphor
