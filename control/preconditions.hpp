#pragma once

#include <algorithm>
#include <phosphor-logging/log.hpp>

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
auto property_states_match(std::vector<PrecondGroup>&& pg,
                           std::vector<SetSpeedEvent>&& sse)
{
    return [pg = std::move(pg),
            sse = std::move(sse)](auto& zone, auto& group)
    {
        // Compare given precondition entries
        auto precondState = std::all_of(
            pg.begin(),
            pg.end(),
            [&zone](auto const& entry)
            {
                try
                {
                    return zone.getPropValueVariant(
                        std::get<pcPathPos>(entry),
                        std::get<pcIntfPos>(entry),
                        std::get<pcPropPos>(entry)) ==
                                std::get<pcValuePos>(entry);
                }
                catch (const std::out_of_range& oore)
                {
                    // Default to property variants not equal when not found
                    return false;
                }
            });

        if (precondState)
        {
            log<level::DEBUG>(
                "Preconditions passed, init the associated events",
                entry("EVENT_COUNT=%u", sse.size()));
            // Init the events when all the precondition(s) are true
            std::for_each(
                sse.begin(),
                sse.end(),
                [&zone](auto const& entry)
                {
                    zone.initEvent(entry);
                });
        }
        else
        {
            log<level::DEBUG>(
                "Preconditions not met for events, remove events",
                entry("EVENT_COUNT=%u", sse.size()));
            // Unsubscribe the events' signals when any precondition is false
            std::for_each(
                sse.begin(),
                sse.end(),
                [&zone](auto const& entry)
                {
                    zone.removeEvent(entry);
                });
            zone.setFullSpeed();
        }
        // Update group's fan control active allowed
        zone.setActiveAllow(&group, (precondState == pg.size()));
    };
}

} // namespace precondition
} // namespace control
} // namespace fan
} // namespace phosphor
