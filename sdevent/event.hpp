#pragma once

#include <chrono>
#include <memory>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <sdbusplus/bus.hpp>
#include <systemd/sd-event.h>
#include <xyz/openbmc_project/Common/error.hpp>

namespace sdevent
{
namespace event
{
namespace io
{
class IO;
} // namespace io

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
    private:
        using InternalFailure = sdbusplus::xyz::openbmc_project::Common::
            Error::InternalFailure;

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
            auto rc = sd_event_loop(evt.get());
            if (rc < 0)
            {
                phosphor::logging::elog<InternalFailure>();
            }
        }

        /** @brief Stop the loop. */
        void exit(int status = 0)
        {
            auto rc = sd_event_exit(evt.get(), status);
            if (rc < 0)
            {
                phosphor::logging::elog<InternalFailure>();
            }
        }

        /** @brief Get the loop exit code. */
        auto getExitStatus()
        {
            int status;
            auto rc = sd_event_get_exit_code(evt.get(), &status);
            if (rc < 0)
            {
                phosphor::logging::elog<InternalFailure>();
            }

            return status;
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
            auto rc = sd_event_now(evt.get(), CLOCK_MONOTONIC, &usec);
            if (rc < 0)
            {
                phosphor::logging::elog<InternalFailure>();
            }

            microseconds d(usec);
            return steady_clock::time_point(d);
        }

        friend class io::IO;

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
    using InternalFailure = sdbusplus::xyz::openbmc_project::Common::
        Error::InternalFailure;

    sd_event* e = nullptr;
    auto rc = sd_event_default(&e);
    if (rc < 0)
    {
        phosphor::logging::elog<InternalFailure>();
    }

    return Event(e, std::false_type());
}

} // namespace event
} // namespace sdevent
