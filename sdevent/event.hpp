#pragma once

#include <chrono>
#include <memory>
#include <sdbusplus/bus.hpp>
#include <systemd/sd-event.h>

// TODO: openbmc/openbmc#1720 - add error handling for sd_event API failures

namespace sdevent
{
namespace event
{
namespace timer
{
class Timer;
} // namespace timer
}
namespace event
{

using EventPtr = sd_event*;
class Event;

/** @brief Get an instance of the 'default' event. */
Event newDefault();

namespace details
{

/** @brief unique_ptr functor to release an event reference. */
struct EventDeleter
{
    void operator()(sd_event* ptr) const
    {
        deleter(ptr);
    }

    decltype(&sd_event_unref) deleter = sd_event_unref;
};

/* @brief Alias 'event' to a unique_ptr type for auto-release. */
using Event = std::unique_ptr<sd_event, EventDeleter>;

} // namespace details

/** @class Event
 *  @brief Provides C++ bindings to the sd_event_* class functions.
 */
class Event
{
    public:
        /* Define all of the basic class operations:
         *     Not allowed:
         *         - Default constructor to avoid nullptrs.
         *         - Copy operations due to internal unique_ptr.
         *     Allowed:
         *         - Move operations.
         *         - Destructor.
         */
        Event() = delete;
        Event(const Event&) = delete;
        Event& operator=(const Event&) = delete;
        Event(Event&&) = default;
        Event& operator=(Event&&) = default;
        ~Event() = default;

        /** @brief Conversion constructor from 'EventPtr'.
         *
         *  Increments ref-count of the event-pointer and releases it when
         *  done.
         */
        explicit Event(EventPtr e);

        /** @brief Constructor for 'Event'.
         *
         *  Takes ownership of the event-pointer and releases it when done.
         */
        Event(EventPtr e, std::false_type);

        /** @brief Release ownership of the stored event-pointer. */
        EventPtr release()
        {
            return evt.release();
        }

        /** @brief Wait indefinitely for new event sources. */
        void loop()
        {
            sd_event_loop(evt.get());
        }

        /** @brief Attach to a DBus loop. */
        void attach(sdbusplus::bus::bus& bus)
        {
            bus.attach_event(evt.get(), SD_EVENT_PRIORITY_NORMAL);
        }

        /** @brief C++ wrapper for sd_event_now. */
        auto now()
        {
            using namespace std::chrono;

            uint64_t usec;
            sd_event_now(evt.get(), CLOCK_MONOTONIC, &usec);
            microseconds d(usec);
            return steady_clock::time_point(d);
        }

        friend class timer::Timer;

    private:

        EventPtr get()
        {
            return evt.get();
        }

        details::Event evt;
};

inline Event::Event(EventPtr l) : evt(sd_event_ref(l))
{

}

inline Event::Event(EventPtr l, std::false_type) : evt(l)
{

}

inline Event newDefault()
{
    sd_event* e = nullptr;
    sd_event_default(&e);
    return Event(e, std::false_type());
}

} // namespace event
} // namespace sdevent
