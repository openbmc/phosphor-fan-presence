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

} // namespace trigger
} // namespace control
} // namespace fan
} // namespace phosphor
