#pragma once

#include <systemd/sd-event.h>

#include <memory>

namespace phosphor
{
namespace fan
{
namespace event
{

/**
 * Custom deleter for sd_event_source
 */
struct EventSourceDeleter
{
    void operator()(sd_event_source* eventSource) const
    {
        sd_event_source_unref(eventSource);
    }
};

using EventSourcePtr = std::unique_ptr<sd_event_source, EventSourceDeleter>;

/**
 * Customer deleter for sd_event
 */
struct EventDeleter
{
    void operator()(sd_event* event) const
    {
        sd_event_unref(event);
    }
};

using EventPtr = std::unique_ptr<sd_event, EventDeleter>;

} // namespace event
} // namespace fan
} // namespace phosphor
