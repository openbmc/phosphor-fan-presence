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
            auto sumTemp = 0;
            std::for_each(
                group.begin(),
                group.end(),
                [&zone, &sumTemp](auto const& entry)
                {
                    sumTemp +=
                        zone.template getPropertyValue<int64_t>(
                                entry.first,
                                std::get<intfPos>(entry.second),
                                std::get<propPos>(entry.second));
                }
            );
            auto avgValue= sumTemp / group.size();
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
 * previsouly read.
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
        auto speed = zone.getDefCeiling();
        if (group.size() != 0)
        {
            auto sumVal = 0;
            std::for_each(
                group.begin(),
                group.end(),
                [&zone, &sumVal](auto const& entry)
                {
                    sumVal +=
                        zone.template getPropertyValue<int64_t>(
                                entry.first,
                                std::get<intfPos>(entry.second),
                                std::get<propPos>(entry.second));
                }
            );
            auto avgValue = sumVal / group.size();
            if (avgValue < zone.getPreKey(avgValue))
            {// Value is decreasing from previous
                for (auto it = val_to_speed.rbegin();
                     it != val_to_speed.rend();
                     ++it)
                {
                    if (it == val_to_speed.rbegin() && avgValue >= it->first)
                    {
                        speed = it->second;
                        break;
                    }
                    auto next = std::next(it, 1);
                    if (next != std::rend(val_to_speed))
                    {
                        if (it->first > avgValue && avgValue >= next->first)
                        {
                            speed = it->second;
                            break;
                        }
                    }
                    else
                    {
                        if (it->first > avgValue)
                        {
                            speed = it->second;
                        }
                    }
                }
            }
            else
            {// Value is increasing from previous
                for (auto it = val_to_speed.begin();
                     it != val_to_speed.end();
                     ++it)
                {
                    if (it == val_to_speed.begin() && avgValue <= it->first)
                    {
                        speed = it->second;
                        break;
                    }
                    auto next = std::next(it, 1);
                    if (next != std::end(val_to_speed))
                    {
                        if (it->first < avgValue && avgValue <= next->first)
                        {
                            speed = it->second;
                            break;
                        }
                    }
                    else
                    {
                        if (it->first < avgValue)
                        {
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
