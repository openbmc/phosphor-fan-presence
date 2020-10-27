#include "../json_parser.hpp"
#include "../power_off_rule.hpp"
#include "mock_power_interface.hpp"

#include <gtest/gtest.h>

using namespace phosphor::fan::monitor;
using json = nlohmann::json;

TEST(PowerOffRuleTest, TestRules)
{
    PowerOffAction::PrePowerOffFunc func;
    sd_event* event;
    sd_event_default(&event);
    sdeventplus::Event sdEvent{event};

    const auto faultConfig = R"(
    {
        "fault_handling":
        {
            "power_off_config": [
                {
                    "type": "hard",
                    "cause": "missing_fan_frus",
                    "count": 2,
                    "delay": 0,
                    "state": "at_pgood"
                },
                {
                    "type": "soft",
                    "cause": "nonfunc_fan_rotors",
                    "count": 3,
                    "delay": 0,
                    "state": "runtime"
                },
                {
                    "type": "soft",
                    "cause": "nonfunc_fan_rotors",
                    "count": 4,
                    "delay": 1,
                    "state": "runtime"
                },
                {
                    "type": "hard",
                    "cause": "missing_fan_frus",
                    "count": 4,
                    "delay": 1,
                    "state": "runtime"
                }
            ]
        }
    })"_json;

    std::shared_ptr<PowerInterfaceBase> powerIface =
        std::make_shared<MockPowerInterface>();

    MockPowerInterface& mockIface =
        static_cast<MockPowerInterface&>(*powerIface);

    EXPECT_CALL(mockIface, hardPowerOff).Times(1);
    EXPECT_CALL(mockIface, softPowerOff).Times(1);

    auto rules = getPowerOffRules(faultConfig, powerIface, func);
    ASSERT_EQ(rules.size(), 4);

    FanHealth health{{"fan0", {false, {true, true}}},
                     {"fan1", {false, {true, true}}}};

    {
        // Check rule 0

        // wrong state, won't be active
        rules[0]->check(PowerRuleState::runtime, health);
        EXPECT_FALSE(rules[0]->active());

        rules[0]->check(PowerRuleState::atPgood, health);
        EXPECT_TRUE(rules[0]->active());

        // Run the event loop, since the timeout is 0 it should
        // run the power off and satisfy the EXPECT_CALL above
        sdEvent.run(std::chrono::milliseconds(1));

        // It doesn't really make much sense to cancel a rule after it
        // powered off, but it should at least say it isn't active.
        rules[0]->cancel();
        EXPECT_FALSE(rules[0]->active());
    }

    {
        // Check the second rule.
        rules[1]->check(PowerRuleState::runtime, health);
        EXPECT_FALSE(rules[1]->active());

        // > 2 nonfunc rotors
        health["fan0"] = {true, {true, false}};
        health["fan1"] = {true, {false, false}};

        rules[1]->check(PowerRuleState::runtime, health);
        EXPECT_TRUE(rules[1]->active());

        // Run the event loop, since the timeout is 0 it should
        // run the power off and satisfy the EXPECT_CALL above
        sdEvent.run(std::chrono::milliseconds(1));
    }

    {
        // Check the third rule.  It has a timeout so long we can
        // cancel it before it runs.
        health["fan0"] = {true, {false, false}};
        health["fan1"] = {true, {false, false}};

        rules[2]->check(PowerRuleState::runtime, health);
        EXPECT_TRUE(rules[2]->active());

        sdEvent.run(std::chrono::milliseconds(1));

        rules[2]->cancel();
        EXPECT_FALSE(rules[2]->active());

        // This will go past the timeout, it should have been canceled so the
        // soft power off won't have run and the EXPECT_CALL above
        // should be happy.
        sdEvent.run(std::chrono::seconds(1));
    }

    {
        // Check the 4th rule. Resolve it before it completes
        health["fan0"] = {false, {true, true}};
        health["fan1"] = {false, {true, true}};
        health["fan2"] = {false, {true, true}};
        health["fan3"] = {false, {true, true}};

        rules[3]->check(PowerRuleState::runtime, health);
        EXPECT_TRUE(rules[3]->active());

        // Won't complete yet
        sdEvent.run(std::chrono::milliseconds(1));

        // Make them present
        health["fan0"] = {true, {true, true}};
        health["fan1"] = {true, {true, true}};
        health["fan2"] = {true, {true, true}};
        health["fan3"] = {true, {true, true}};

        //  It should be inactive now
        rules[3]->check(PowerRuleState::runtime, health);
        EXPECT_FALSE(rules[3]->active());
    }
}
