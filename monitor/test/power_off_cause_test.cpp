#include "../power_off_cause.hpp"

#include <gtest/gtest.h>

using namespace phosphor::fan::monitor;

TEST(PowerOffCauseTest, MissingFanTest)
{
    FanHealth health{{"fan0", {true, {true, true}}},
                     {"fan1", {true, {true, true}}},
                     {"fan2", {true, {true, true}}},
                     {"fan3", {true, {true, true}}}};

    MissingFanFRUCause cause{2};
    EXPECT_FALSE(cause.satisfied(health));

    health["fan0"] = {false, {false, false}};
    EXPECT_FALSE(cause.satisfied(health));

    health["fan1"] = {false, {false, false}};
    EXPECT_TRUE(cause.satisfied(health));

    health["fan2"] = {false, {false, false}};
    EXPECT_TRUE(cause.satisfied(health));

    health["fan0"] = {false, {true, true}};
    health["fan1"] = {false, {true, true}};
    health["fan2"] = {false, {true, true}};
    EXPECT_TRUE(cause.satisfied(health));
}

TEST(PowerOffCauseTest, NonfuncRotorTest)
{
    FanHealth health{{"fan0", {true, {true, true}}},
                     {"fan1", {true, {true, true}}},
                     {"fan2", {true, {true, true}}},
                     {"fan3", {true, {true, true}}}};

    NonfuncFanRotorCause cause{2};
    EXPECT_FALSE(cause.satisfied(health));

    health["fan0"] = {true, {true, false}};
    EXPECT_FALSE(cause.satisfied(health));

    health["fan1"] = {true, {false, true}};
    EXPECT_TRUE(cause.satisfied(health));

    health["fan2"] = {true, {true, false}};
    EXPECT_TRUE(cause.satisfied(health));

    health["fan0"] = {false, {true, true}};
    health["fan1"] = {false, {true, true}};
    health["fan2"] = {false, {true, true}};
    EXPECT_FALSE(cause.satisfied(health));
}
