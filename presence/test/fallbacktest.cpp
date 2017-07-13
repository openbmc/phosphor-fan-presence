#include <iostream>
#include <string>
#include <gtest/gtest.h>
#include "../fallback.hpp"
#include "../psensor.hpp"

int pstate = -1;

namespace phosphor
{
namespace fan
{
namespace presence
{
void setPresence(const Fan& fan, bool newState)
{
    if (newState)
        pstate = 1;
    else
        pstate = 0;
}

struct TestSensor : public PresenceSensor
{
    bool start() override
    {
        ++started;
        return _start;
    }

    void stop() override
    {
        ++stopped;
    }

    bool present() override
    {
        return _present;
    }

    void fail() override
    {
        ++failed;
    }

    bool _start = false;
    bool _present = false;
    size_t started = 0;
    size_t stopped = 0;
    size_t failed = 0;
};

} // namespace presence
} // namespace fan
} // namespace phosphor

using namespace phosphor::fan::presence;

TEST(FallbackTest, TestOnePresent)
{
    pstate = -1;
    Fan fan{"/path", "name"};
    TestSensor ts;
    ts._start = true;
    ts._present = true;
    std::vector<std::reference_wrapper<PresenceSensor>> sensors{ts};
    Fallback f{fan, sensors};

    f.monitor();
    ASSERT_EQ(pstate, 1);
}

TEST(FallbackTest, TestOneNotPresent)
{
    pstate = -1;
    Fan fan{"/path", "name"};
    TestSensor<false, false> ts;
    std::vector<std::reference_wrapper<PresenceSensor>> sensors{ts};
    Fallback f{fan, sensors};

    f.monitor();
    ASSERT_EQ(pstate, 0);
}

TEST(FallbackTest, TestTwoPresent)
{
    pstate = -1;
    Fan fan{"/path", "name"};
    TestSensor<true, true> ts1;
    TestSensor<true, true> ts2;
    std::vector<std::reference_wrapper<PresenceSensor>> sensors{ts1, ts2};
    Fallback f{fan, sensors};

    f.monitor();
    ASSERT_EQ(pstate, 1);
}

TEST(FallbackTest, TestMismatch)
{
    pstate = -1;
    Fan fan{"/path", "name"};
    TestSensor<false, false> ts1;
    TestSensor<false, false> ts2;
    std::vector<std::reference_wrapper<PresenceSensor>> sensors{ts1, ts2};
    Fallback f{fan, sensors};

    f.monitor();
    ASSERT_EQ(pstate, 0);
}
