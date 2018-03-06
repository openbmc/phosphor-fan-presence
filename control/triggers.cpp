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

} // namespace trigger
} // namespace control
} // namespace fan
} // namespace phosphor
