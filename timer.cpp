/**
 * Copyright © 2017 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <chrono>
#include <phosphor-logging/log.hpp>
#include "timer.hpp"

namespace phosphor
{
namespace fan
{
namespace util
{

using namespace phosphor::logging;

Timer::Timer(sd_event* events,
             std::function<void()> callbackFunc) :
    _timeEvent(events),
    _callback(callbackFunc),
    _timeout(0)
{

    // Start with an infinite expiration time
    auto r = sd_event_add_time(_timeEvent,
                               &_eventSource,
                               CLOCK_MONOTONIC, // Time base
                               UINT64_MAX,      // Expire time - way long time
                               0,               // Use default event accuracy
                               timeoutHandler,  // Callback handler on timeout
                               this);           // User data
    if (r < 0)
    {
        log<level::ERR>("Timer::Timer failed call to sd_event_add_time",
                        entry("ERROR=%s", strerror(-r)));
        throw std::runtime_error("Timer initialization failed");
    }

    //Ensure timer isn't running
    setTimer(SD_EVENT_OFF);
}


int Timer::timeoutHandler(sd_event_source* eventSource,
                          uint64_t usec, void* userData)
{
    auto timer = static_cast<Timer*>(userData);

    timer->_callback();

    return 0;
}


std::chrono::microseconds Timer::getTime()
{
    using namespace std::chrono;
    auto usec = steady_clock::now().time_since_epoch();
    return duration_cast<microseconds>(usec);
}


void Timer::setTimer(int action)
{
    auto r = sd_event_source_set_enabled(_eventSource, action);
    if (r < 0)
    {
        log<level::ERR>("Failed call to sd_event_source_set_enabled",
                        entry("ERROR=%s", strerror(-r)),
                        entry("ACTION=%d", action));
        throw std::runtime_error("Failed call to sd_event_source_set_enabled");
    }
}


void Timer::stop()
{
    setTimer(SD_EVENT_OFF);
}


bool Timer::running()
{
    int status = 0;

    //returns SD_EVENT_OFF, SD_EVENT_ON, or SD_EVENT_ONESHOT
    auto r = sd_event_source_get_enabled(_eventSource, &status);
    if (r < 0)
    {
        log<level::ERR>("Failed call to sd_event_source_get_enabled",
                        entry("ERROR=%s", strerror(-r)));
        throw std::runtime_error("Failed call to sd_event_source_get_enabled");
    }

    return (status != SD_EVENT_OFF);
}


void Timer::start(std::chrono::microseconds timeValue)
{
    // Disable the timer
    setTimer(SD_EVENT_OFF);

    _timeout = timeValue;

    // Get the current time and add the delta
    auto expireTime = getTime() + timeValue;

    // Set the time
    auto r = sd_event_source_set_time(_eventSource, expireTime.count());
    if (r < 0)
    {
        log<level::ERR>("Failed call to sd_event_source_set_time",
                        entry("ERROR=%s", strerror(-r)));
        throw std::runtime_error("Failed call to sd_event_source_set_time");
    }

    // A ONESHOT timer means that when the timer goes off,
    // its moves to disabled state.
    setTimer(SD_EVENT_ONESHOT);
}


}
}
}
