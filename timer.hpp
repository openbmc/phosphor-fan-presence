#pragma once

#include <chrono>
#include <functional>
#include <systemd/sd-event.h>

namespace phosphor
{
namespace fan
{
namespace util
{


/**
 * @class Timer
 *
 * This class implements a simple timer that runs an arbitrary
 * function on expiration.  The timeout value is set in microseconds.
 * It can be stopped while it is running, and queried to see if it is
 * running.  It needs an sd_event loop to function.
 */
class Timer
{
    public:

        Timer() = delete;
        ~Timer() = default;
        Timer(const Timer&) = delete;
        Timer& operator=(const Timer&) = delete;
        Timer(Timer&&) = default;
        Timer& operator=(Timer&&) = default;

        /**
         * @brief Constructs timer object
         *
         *  @param[in] events - sd_event pointer, previously created
         *  @param[in] callbackFunc - The function to call on timer expiration
         */
        Timer(sd_event* events,
              std::function<void()> callbackFunc);

        /**
         * @brief starts the timer
         *
         * @param[in] usec - the timeout value in microseconds
         */
        void start(std::chrono::microseconds usec);

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
            return _timeout;
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
         * @brief The sd_event structure
         */
        sd_event* _timeEvent;

        /**
         * @brief Source of events
         */
        sd_event_source* _eventSource = nullptr;

        /**
         * @brief The function to call when the timer expires
         */
        std::function<void()> _callback;

        /**
         * @brief What the timer was set to run for
         *
         * Not cleared on timer expiration
         */
        std::chrono::microseconds _timeout;
};

}
}
}
