#include "triggers.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{
namespace trigger
{

using namespace phosphor::fan;

Trigger timer(TimerConf&& tConf)
{
    return [tConf = std::move(tConf)](control::Zone& zone,
                                      const Group& group,
                                      const std::vector<Action>& actions)
    {
        zone.addTimer(group,
                      actions,
                      tConf);
    };
}

Trigger signal(const std::string& match, SignalHandler&& handler)
{
    return [match = std::move(match),
            handler = std::move(handler)](control::Zone& zone,
                                          const Group& group,
                                          const std::vector<Action>& actions)
    {
        // Setup signal matches of the property for event
        std::unique_ptr<EventData> eventData =
            std::make_unique<EventData>(
                    group,
                    match,
                    handler,
                    actions
            );
        std::unique_ptr<sdbusplus::server::match::match> mPtr = nullptr;
        if (!match.empty())
        {
            // Subscribe to signal match
            mPtr = std::make_unique<sdbusplus::server::match::match>(
                    zone.getBus(),
                    match.c_str(),
                    std::bind(std::mem_fn(&Zone::handleEvent),
                              &zone,
                              std::placeholders::_1,
                              eventData.get())
            );
        }
        zone.addSignal(std::move(eventData), std::move(mPtr));
    };
}

Trigger init(MethodHandler&& handler)
{
    return [handler = std::move(handler)](control::Zone& zone,
                                          const Group& group,
                                          const std::vector<Action>& actions)
    {
        // A handler function is optional
        if (handler)
        {
            handler(zone, group);
        }

        // Run action functions for initial event state
        std::for_each(
            actions.begin(),
            actions.end(),
            [&zone, &group](auto const& action)
            {
                // TODO Use action groups instead of event groups if exists
                // Similarly on the timer and signal events
                action(zone, group);
            }
        );
    };
}

} // namespace trigger
} // namespace control
} // namespace fan
} // namespace phosphor
