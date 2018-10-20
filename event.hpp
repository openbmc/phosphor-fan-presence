#pragma once

#include <memory>
#include <systemd/sd-event.h>

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

}
}
}
