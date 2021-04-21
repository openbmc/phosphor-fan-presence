#include "power_state.hpp"

namespace phosphor::fan
{

std::shared_ptr<PowerState> powerState;

std::shared_ptr<PowerState> getPowerStateObject()
{
    if (!powerState)
    {
        powerState = std::make_shared<PGoodState>();
    }
    return powerState;
}

} // namespace phosphor::fan
