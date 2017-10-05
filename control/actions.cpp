#include "actions.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{
namespace action
{

using namespace phosphor::fan;

Action call_actions_based_on_timer(Timer&& tConf, std::vector<Action>&& actions)
{
    return [tConf = std::move(tConf),
            actions = std::move(actions)](control::Zone& zone,
                                          const Group& group)
    {
        try
        {
            // Find any services that do not have an owner
            auto services = zone.getGroupServices(&group);
            auto setTimer = std::any_of(
                services.begin(),
                services.end(),
                [](const auto& s)
                {
                    return !std::get<hasOwnerPos>(s);
                });
            if (setTimer &&
                zone.findTimer(group, actions) ==
                    std::end(zone.getTimerEvents()))
            {
                // Associate event data with timer
                std::unique_ptr<EventData> eventData =
                    std::make_unique<EventData>(
                            EventData
                            {
                                group,
                                nullptr,
                                actions
                            }
                    );
                // Create/start timer and associate event data with it
                std::unique_ptr<util::Timer> timer =
                    std::make_unique<util::Timer>(
                            zone.getEventPtr(),
                            [&zone,
                            actions = &actions,
                            group = &group]()
                            {
                                zone.timerExpired(*group, *actions);
                            });
                if (!timer->running())
                {
                    timer->start(std::get<intervalPos>(tConf),
                                 std::get<typePos>(tConf));
                }
                zone.addTimer(std::move(eventData), std::move(timer));
            }
            else
            {
                // Stop and remove any timers for this group
                auto timer = zone.findTimer(group, actions);
                if (timer != std::end(zone.getTimerEvents()))
                {
                    if (std::get<timerTimerPos>(*timer)->running())
                    {
                        std::get<timerTimerPos>(*timer)->stop();
                    }
                    zone.removeTimer(timer);
                }
            }
        }
        catch (const std::out_of_range& oore)
        {
            // Group not found, no timers set
        }
    };
}

void set_request_speed_base_with_max(control::Zone& zone,
                                     const Group& group)
{
    int64_t base = 0;
    std::for_each(
            group.begin(),
            group.end(),
            [&zone, &base](auto const& entry)
        {
            try
            {
                auto value = zone.template getPropertyValue<int64_t>(
                        entry.first,
                        std::get<intfPos>(entry.second),
                        std::get<propPos>(entry.second));
                base = std::max(base, value);
            }
            catch (const std::out_of_range& oore)
            {
                // Property value not found, base request speed unchanged
            }
        });
    // A request speed base of 0 defaults to the current target speed
    zone.setRequestSpeedBase(base);
}

Action set_floor_from_average_sensor_value(
        std::map<int64_t, uint64_t>&& val_to_speed)
{
    return [val_to_speed = std::move(val_to_speed)](control::Zone& zone,
                                                    const Group& group)
    {
        auto speed = zone.getDefFloor();
        if (group.size() != 0)
        {
            auto count = 0;
            auto sumValue = std::accumulate(
                    group.begin(),
                    group.end(),
                    0,
                    [&zone, &count](int64_t sum, auto const& entry)
                    {
                        try
                        {
                            return sum +
                                zone.template getPropertyValue<int64_t>(
                                    entry.first,
                                    std::get<intfPos>(entry.second),
                                    std::get<propPos>(entry.second));
                        }
                        catch (const std::out_of_range& oore)
                        {
                            count++;
                            return sum;
                        }
                    });
            if ((group.size() - count) > 0)
            {
                auto groupSize = static_cast<int64_t>(group.size());
                auto avgValue = sumValue / (groupSize - count);
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
        }
        zone.setFloor(speed);
    };
}

Action set_ceiling_from_average_sensor_value(
        std::map<int64_t, uint64_t>&& val_to_speed)
{
    return [val_to_speed = std::move(val_to_speed)](Zone& zone,
                                                    const Group& group)
    {
        auto speed = zone.getCeiling();
        if (group.size() != 0)
        {
            auto count = 0;
            auto sumValue = std::accumulate(
                    group.begin(),
                    group.end(),
                    0,
                    [&zone, &count](int64_t sum, auto const& entry)
                    {
                        try
                        {
                            return sum +
                                zone.template getPropertyValue<int64_t>(
                                    entry.first,
                                    std::get<intfPos>(entry.second),
                                    std::get<propPos>(entry.second));
                        }
                        catch (const std::out_of_range& oore)
                        {
                            count++;
                            return sum;
                        }
                    });
            if ((group.size() - count) > 0)
            {
                auto groupSize = static_cast<int64_t>(group.size());
                auto avgValue = sumValue / (groupSize - count);
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
                                // Value is at/above last map key, set
                                // ceiling speed to the last map key's value
                                speed = it->second;
                                break;
                            }
                            else if (std::next(it, 1) == val_to_speed.rend() &&
                                     avgValue <= it->first)
                            {
                                // Value is at/below first map key, set
                                // ceiling speed to the first map key's value
                                speed = it->second;
                                break;
                            }
                            if (avgValue < it->first &&
                                it->first <= prevValue)
                            {
                                // Value decreased & transitioned across
                                // a map key, update ceiling speed to this
                                // map key's value when new value is below
                                // map's key and the key is at/below the
                                // previous value
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
                                // Value is at/below first map key, set
                                // ceiling speed to the first map key's value
                                speed = it->second;
                                break;
                            }
                            else if (std::next(it, 1) == val_to_speed.end() &&
                                     avgValue >= it->first)
                            {
                                // Value is at/above last map key, set
                                // ceiling speed to the last map key's value
                                speed = it->second;
                                break;
                            }
                            if (avgValue > it->first &&
                                it->first >= prevValue)
                            {
                                // Value increased & transitioned across
                                // a map key, update ceiling speed to this
                                // map key's value when new value is above
                                // map's key and the key is at/above the
                                // previous value
                                speed = it->second;
                            }
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
