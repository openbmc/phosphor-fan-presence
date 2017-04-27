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
#include <iostream>
#include <chrono>
#include <gtest/gtest.h>
#include "timer.hpp"

/**
 * Testcases for the Timer class
 */

using namespace phosphor::fan::util;
using namespace std::chrono;

/**
 * Class to ensure sd_events are correctly
 * setup and destroyed.
 */
class TimerTest : public ::testing::Test
{
    public:
        // systemd event handler
        sd_event* events;

        // Need this so that events can be initialized.
        int rc;

        // Gets called as part of each TEST_F construction
        TimerTest() : rc(sd_event_default(&events))
        {
            EXPECT_GE(rc, 0);
        }

        // Gets called as part of each TEST_F destruction
        ~TimerTest()
        {
            events = sd_event_unref(events);
        }
};

/**
 * Helper class to hande tracking timer expirations
 * via callback functions.
 */
class CallbackTester
{
    public:

        CallbackTester() {}

        size_t getCount()
        {
            return _count;
        }

        void callbackFunction()
        {
            _count++;
            _gotCallback = true;
        }

        bool gotCallback()
        {
            return _gotCallback;
        }

    private:
        bool _gotCallback = false;
        size_t _count = 0;
};


/**
 * Helper class that more closely mimics real usage,
 * which is another class containing a timer and using
 * one of its member functions as the callback.
 */
class CallbackTesterWithTimer : public CallbackTester
{
    public:
        CallbackTesterWithTimer(sd_event* events) :
            _timer(events,
                   std::bind(&CallbackTesterWithTimer::callbackFunction,
                             this))
        {
        }

        void callbackFunction()
        {
            //restart the timer once from the callback
            if (!_restarted)
            {
                _restarted = true;
                auto time = duration_cast<microseconds>(seconds(1));
                _timer.start(time);
            }

            CallbackTester::callbackFunction();
        }

        Timer& getTimer()
        {
            return _timer;
        }

    private:

        Timer _timer;
        bool _restarted = false;
};


/**
 * Test that a callback will occur after 2 seconds.
 */
TEST_F(TimerTest, timerExpiresAfter2seconds)
{
    CallbackTester tester;

    Timer timer(events,
                std::bind(&CallbackTester::callbackFunction, &tester));


    auto time = duration_cast<microseconds>(seconds(2));

    EXPECT_EQ(false, timer.running());

    timer.start(time);
    EXPECT_EQ(false, tester.gotCallback());
    EXPECT_EQ(true, timer.running());

    int count = 0;
    auto sleepTime = duration_cast<microseconds>(seconds(1));

    //Wait for 2 1s timeouts
    while (count < 2)
    {
        // Returns 0 on timeout and positive number on dispatch
        if (sd_event_run(events, sleepTime.count()) == 0)
        {
            count++;
        }
    }

    EXPECT_EQ(true, tester.gotCallback());
    EXPECT_EQ(1, tester.getCount());
    EXPECT_EQ(false, timer.running());
}

/**
 * Test that a timer can be restarted.
 */
TEST_F(TimerTest, timerRestart)
{
    CallbackTester tester;

    Timer timer(events,
                std::bind(&CallbackTester::callbackFunction, &tester));


    auto time = duration_cast<microseconds>(seconds(2));
    timer.start(time);

    //wait for a second
    auto sleepTime = duration_cast<microseconds>(seconds(1));
    auto rc = sd_event_run(events, sleepTime.count());

    //expect the timeout, not the dispatch
    //and the timer should still be running
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, timer.running());

    //Restart it
    timer.start(time);

    //Wait just 1s, make sure not done
    rc = sd_event_run(events, sleepTime.count());
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, timer.running());
    EXPECT_EQ(false, tester.gotCallback());

    //Wait 1 more second, this time expecting a dispatch
    int count = 0;
    while (count < 1)
    {
        // Returns 0 on timeout and positive number on dispatch
        if (sd_event_run(events, sleepTime.count()) == 0)
        {
            count++;
        }
    }

    EXPECT_EQ(true, tester.gotCallback());
    EXPECT_EQ(1, tester.getCount());
    EXPECT_EQ(false, timer.running());
}


/**
 * Test that a timer can be stopped.
 */
TEST_F(TimerTest, timerStop)
{
    CallbackTester tester;

    Timer timer(events,
                std::bind(&CallbackTester::callbackFunction, &tester));


    auto time = duration_cast<microseconds>(seconds(2));
    timer.start(time);

    auto sleepTime = duration_cast<microseconds>(seconds(1));

    //wait 1s
    auto rc = sd_event_run(events, sleepTime.count());

    //expect the timeout, not the dispatch
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(true, timer.running());

    timer.stop();

    EXPECT_EQ(false, timer.running());
    EXPECT_EQ(false, tester.gotCallback());

    //Wait another 2s, make sure no callbacks happened
    sleepTime = duration_cast<microseconds>(seconds(2));
    rc = sd_event_run(events, sleepTime.count());

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(false, timer.running());
    EXPECT_EQ(false, tester.gotCallback());
}


/**
 * Test that the timer can be restarted from within
 * a callback function.
 */
TEST_F(TimerTest, timerRestartFromCallback)
{
    CallbackTesterWithTimer tester(events);

    auto& timer = tester.getTimer();

    auto time = duration_cast<microseconds>(seconds(2));
    timer.start(time);

    //after running for 2 seconds, the timer will get restarted
    //for another 1s

    int count = 0;
    auto sleepTime = duration_cast<microseconds>(seconds(1));
    while (count < 3)
    {
        // Returns 0 on timeout and positive number on dispatch
        if (sd_event_run(events, sleepTime.count()) == 0)
        {
            count++;
        }
    }

    EXPECT_EQ(false, timer.running());
    EXPECT_EQ(true, tester.gotCallback());
    EXPECT_EQ(2, tester.getCount()); //2 callbacks
}

/**
 * This shows what happens when the timer expires but
 * sd_event_run never got called.
 */
TEST_F(TimerTest, timerNoEventRun)
{
    CallbackTester tester;

    Timer timer(events,
                std::bind(&CallbackTester::callbackFunction, &tester));


    auto time = duration_cast<microseconds>(milliseconds(500));

    timer.start(time);

    sleep(1);

    //The timer should have expired, but with no event processing
    //it will still think it's running.

    EXPECT_EQ(true, timer.running());
    EXPECT_EQ(false, tester.gotCallback());

    //Now process an event
    auto sleepTime = duration_cast<microseconds>(milliseconds(5));
    auto rc = sd_event_run(events, sleepTime.count());

    EXPECT_GT(rc, 0);
    EXPECT_EQ(false, timer.running());
    EXPECT_EQ(true, tester.gotCallback());
}

