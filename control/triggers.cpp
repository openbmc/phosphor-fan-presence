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

Trigger timer(Timer&& tConf)
{
    return [tConf = std::move(tConf)](control::Zone& zone,
                                      const Group& group,
                                      const std::vector<Action>& actions)
    {
        // Associate event data with timer
        std::unique_ptr<EventData> eventData =
            std::make_unique<EventData>(
                    group,
                    "",
                    nullptr,
                    actions
            );
        // Create timer for the group's actions
        std::unique_ptr<util::Timer> timer =
            std::make_unique<util::Timer>(
                zone.getEventPtr(),
                [z = &zone, a = &actions, g = &group]()
                {
                    z->timerExpired(*g, *a);
                });
        if (!timer->running())
        {
            timer->start(std::get<intervalPos>(tConf),
                         std::get<typePos>(tConf));
        }
        zone.addTimer(std::move(eventData), std::move(timer));
    };
}

Trigger signal(const std::string& match, Handler&& handler)
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

Trigger init(Handler&& handler)
{
    return [handler = std::move(handler)](control::Zone& zone,
                                          const Group& group,
                                          const std::vector<Action>& actions)
    {
        // A handler function is optional
        if (handler)
        {
            sdbusplus::message::message nullMsg{nullptr};
            // Execute the given handler function prior to running the actions
            handler(zone.getBus(), nullMsg, zone);
        }
        // Run action functions for initial event state
        std::for_each(
            actions.begin(),
            actions.end(),
            [&zone, &group](auto const& action)
            {
                action(zone, group);
            }
        );
    };
}

} // namespace trigger
} // namespace control
} // namespace fan
} // namespace phosphor
