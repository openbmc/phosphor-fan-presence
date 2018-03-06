#pragma once

#include "types.hpp"
#include "zone.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{
namespace trigger
{

/**
 * @brief A trigger to start a timer for an event
 * @details Creates and starts a timer according to the configuration given
 * that will call an event's actions upon each timer expiration.
 *
 * @param[in] tConf - Timer configuration parameters
 *
 * @return Trigger lambda function
 *     A Trigger function that creates and starts a timer
 */
Trigger timer(Timer&& tConf);

} // namespace trigger
} // namespace control
} // namespace fan
} // namespace phosphor
