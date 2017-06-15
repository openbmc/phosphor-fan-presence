#pragma once

#include "sdevent/event.hpp"

namespace phosphor
{
namespace fan
{
namespace util
{
/** @class SDEvent
 *  @brief DBus access delegate implementation for sdevent.
 */
class SDEvent
{
    public:
        /** @brief Get the event loop. */
        static auto& getEvent()
        {
            static auto event = sdevent::event::newDefault();
            return event;
        }
};

} // namespace util
} // namespace fan
} // namespace phosphor
