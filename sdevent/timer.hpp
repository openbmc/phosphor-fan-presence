#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <systemd/sd-event.h>

#include "sdevent/source.hpp"
#include "sdevent/event.hpp"

// TODO: openbmc/openbmc#1720 - add error handling for sd_event API failures

namespace sdevent
{
namespace event
{
namespace timer
{

/** @class Timer
 *  @brief Provides C++ bindings to the sd_event_source* timer functions.
 */
class Timer
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
        Timer() = delete;
        Timer(const Timer&) = delete;
        Timer& operator=(const Timer&) = delete;
        Timer(Timer&&) = default;
        Timer& operator=(Timer&&) = default;
        ~Timer() = default;

        using Callback = std::function<void(source::Source&)>;

        /** @brief Register a timer callback.
         *
         *  @param[in] event - The event to register on.
         *  @param[in] expires - The initial timer expiration time.
         *  @param[in] callback - The callback method.
         */
        Timer(
            sdevent::event::Event& event,
            const std::chrono::steady_clock::time_point& expires,
            Callback callback)
            : src(nullptr),
              cb(std::make_unique<Callback>(std::move(callback)))
        {
            using namespace std::chrono;
            auto epoch = expires.time_since_epoch();
            auto time = duration_cast<microseconds>(epoch);
            sd_event_source* source = nullptr;
            sd_event_add_time(
                event.get(),
                &source,
                CLOCK_MONOTONIC,
                time.count(),
                0,
                callCallback,
                cb.get());
// **INDENT-OFF**
            src = decltype(src){source, std::false_type()};
// **INDENT-ON**
        }

        /** @brief Register a disabled timer callback.
         *
         *  @param[in] event - The event to register on.
         *  @param[in] callback - The callback method.
         */
        Timer(
            sdevent::event::Event& event,
            Callback callback)
            : src(nullptr),
              cb(std::make_unique<Callback>(std::move(callback)))
        {
            sd_event_source* source = nullptr;
            sd_event_add_time(
                event.get(),
                &source,
                CLOCK_MONOTONIC,
                ULLONG_MAX,
                0,
                callCallback,
                cb.get());
// **INDENT-OFF**
            src = decltype(src){source, std::false_type()};
// **INDENT-ON**
        }

        /** @brief Set the timer expiration time. */
        void setTime(
            const std::chrono::steady_clock::time_point& expires)
        {
            src.setTime(expires);
        }

        /** @brief Get the timer expiration time. */
        auto getTime()
        {
            return src.getTime();
        }

        /** @brief Set the timer source enable state. */
        void enable(int enable)
        {
            src.enable(enable);
        }

        /** @brief Query timer state. */
        auto enabled()
        {
            return src.enabled();
        }

    private:
        source::Source src;
        std::unique_ptr<Callback> cb = nullptr;

        static int callCallback(sd_event_source* s, uint64_t usec,
                                void* context)
        {
            source::Source source(s);
            auto c = static_cast<Callback*>(context);
            (*c)(source);

            return 0;
        }
};
} // namespace timer
} // namespace event
} // namespace sdevent
