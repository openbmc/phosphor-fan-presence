#pragma once

#include "power_state.hpp"

namespace phosphor::fan
{

/**
 * @brief Returns the PowerState object as a shared_ptr.
 *
 * Callers can use addCallback() on the return object to
 * have functions run when the power state changes.
 *
 * @return shared_ptr<PowerState>
 */
std::shared_ptr<PowerState> getPowerStateObject();

} // namespace phosphor::fan
