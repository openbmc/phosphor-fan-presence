#pragma once

#include <chrono>
#include <memory>
#include <systemd/sd-event.h>

// TODO: openbmc/openbmc#1720 - add error handling for sd_event API failures

namespace sdevent
{
namespace source
{

using SourcePtr = sd_event_source*;

namespace details
{

/** @brief unique_ptr functor to release a source reference. */
struct SourceDeleter
{
    void operator()(sd_event_source* ptr) const
    {
        deleter(ptr);
    }

    decltype(&sd_event_source_unref) deleter = sd_event_source_unref;
};

/* @brief Alias 'source' to a unique_ptr type for auto-release. */
using source = std::unique_ptr<sd_event_source, SourceDeleter>;

} // namespace details

/** @class Source
 *  @brief Provides C++ bindings to the sd_event_source* functions.
 */
class Source
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
        Source() = delete;
        Source(const Source&) = delete;
        Source& operator=(const Source&) = delete;
        Source(Source&&) = default;
        Source& operator=(Source&&) = default;
        ~Source() = default;

        /** @brief Conversion constructor from 'SourcePtr'.
         *
         *  Increments ref-count of the source-pointer and releases it
         *  when done.
         */
        explicit Source(SourcePtr s) : src(sd_event_source_ref(s)) {}

        /** @brief Constructor for 'source'.
         *
         *  Takes ownership of the source-pointer and releases it when done.
         */
        Source(SourcePtr s, std::false_type) : src(s) {}

        /** @brief Check if source contains a real pointer. (non-nullptr). */
        explicit operator bool() const
        {
            return bool(src);
        }

        /** @brief Test whether or not the source can generate events. */
        auto enabled()
        {
            int enabled;
            sd_event_source_get_enabled(src.get(), &enabled);
            return enabled;
        }

        /** @brief Allow the source to generate events. */
        void enable(int enable)
        {
            sd_event_source_set_enabled(src.get(), enable);
        }

        /** @brief Set the expiration on a timer source. */
        void setTime(
            const std::chrono::steady_clock::time_point& expires)
        {
            using namespace std::chrono;
            auto epoch = expires.time_since_epoch();
            auto time = duration_cast<microseconds>(epoch);
            sd_event_source_set_time(src.get(), time.count());
        }

        /** @brief Get the expiration on a timer source. */
        auto getTime()
        {
            using namespace std::chrono;
            uint64_t usec;
            sd_event_source_get_time(src.get(), &usec);
            microseconds d(usec);
            return steady_clock::time_point(d);
        }

    private:
        details::source src;
};
} // namespace source
} // namespace sdevent
