#include "control/json/profile.hpp"

#include <iostream>
#include <string>

#include <gtest/gtest.h>

using namespace phosphor::fan::control::json;

TEST(ProfileTest, Test)
{
    auto bus = sdbusplus::bus::new_default();

    {
        static const auto json = R"(
        {
            "name": "Test",
            "method": {
                "name": "all_of",
                "properties": []
            }
        }
        )"_json;

        Profile profile{bus, json};
    }

    {
        static const auto json = R"(
        {
            "name": "Test"
        }
        )"_json;

        EXPECT_THROW(Profile profile(bus, json),  std::runtime_error);
    }
}
