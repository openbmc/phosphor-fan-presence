#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <systemd/sd-event.h>

namespace phosphor
{
namespace fan
{
namespace util
{

struct EventSourceDeleter
{
    void operator()(sd_event_source* eventSource) const
    {
        sd_event_source_unref(eventSource);
    }
};

using EventSourcePtr = std::unique_ptr<sd_event_source, EventSourceDeleter>;

using EventPtr = std::shared_ptr<sd_event>;

/**
 * @class Timer
 *
 * This class implements a simple timer that runs an arbitrary
 * function on expiration.  The timeout value is set in microseconds.
 * It can be stopped while it is running, and queried to see if it is
 * running.
 *
 * If started with the 'repeating' argument, it will keep calling the
 * callback function every <timeout> microseconds.  If started with the
 * 'oneshot' argument, it will just call the callback function one time.
 *
 * It needs an sd_event loop to function.
 */
class Timer
{
    public:

        enum class TimerType
        {
            oneshot,
            repeating
        };

        Timer() = delete;
        Timer(const Timer&) = delete;
        Timer& operator=(const Timer&) = delete;
        Timer(Timer&&) = default;
        Timer& operator=(Timer&&) = default;

        /**
         * @brief Constructs timer object
         *
         * @param[in] events - sd_event pointer, previously created
         * @param[in] callbackFunc - The function to call on timer expiration
         */
        Timer(EventPtr& events,
              std::function<void()> callbackFunc);

        /**
         * @brief Destructor
         */
        ~Timer();

        /**
         * @brief Starts the timer
         *
         * The input is an offset from the current steady clock.
         *
         * @param[in] usec - the timeout value in microseconds
         * @param[in] type - either a one shot, or repeating
         */
        void start(std::chrono::microseconds usec,
                   TimerType type);

        /**
         * @brief Stop the timer
         */
        void stop();

        /**
         * @brief Returns true if the timer is running
         */
        bool running();

        /**
         * @brief Returns the timeout value
         *
         *  Returns the last value sent in via start().
         *  Could be used to restart the timer with the same
         *  timeout.  i.e.  start(getTimeout());
         */
         inline auto getTimeout()
         {
            return timeout;
         }

        /**
         * @brief Returns the timer type
         */
        inline auto getType()
        {
            return type;
        }

    private:

        /**
         * @brief Callback function when timer goes off
         *
         * Calls the callback function passed in by the user.
         *
         * @param[in] eventSource - Source of the event
         * @param[in] usec        - time in micro seconds
         * @param[in] userData    - User data pointer
         */
        static int timeoutHandler(sd_event_source* eventSource,
                                  uint64_t usec, void* userData);

        /**
         * @brief Gets the current time from the steady clock
         */
        std::chrono::microseconds getTime();

        /**
         * @brief Wrapper around sd_event_source_set_enabled
         *
         * @param[in] action - either SD_EVENT_OFF, SD_EVENT_ON,
         *                     or SD_EVENT_ONESHOT
         */
        void setTimer(int action);


        /**
         * @brief Sets the expiration time for the timer
         *
         * Sets it to timeout microseconds in the future
         */
        void setTimeout();

        /**
         * @brief The sd_event structure
         */
        EventPtr timeEvent;

        /**
         * @brief Source of events
         */
        EventSourcePtr eventSource;

        /**
         * @brief Either 'repeating' or 'oneshot'
         */
        TimerType type = TimerType::oneshot;

        /**
         * @brief The function to call when the timer expires
         */
        std::function<void()> callback;

        /**
         * @brief What the timer was set to run for
         *
         * Not cleared on timer expiration
         */
        std::chrono::microseconds timeout;
};

}
}
}
