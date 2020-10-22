#include "logger.hpp"

#include <gtest/gtest.h>

using namespace phosphor::fan;
using namespace std::literals::string_literals;

TEST(LoggerTest, Test)
{
    const auto logSize = 5;

    Logger logger{logSize};

    for (int i = 0; i < logSize; i++)
    {
        logger.log("Test Message "s + std::to_string(i));
    }

    auto messages = logger.getLogs();

    ASSERT_EQ(messages.size(), logSize);

    EXPECT_EQ((messages[0][1].get<std::string>()), "Test Message 0");
    EXPECT_EQ((messages[1][1].get<std::string>()), "Test Message 1");
    EXPECT_EQ((messages[2][1].get<std::string>()), "Test Message 2");
    EXPECT_EQ((messages[3][1].get<std::string>()), "Test Message 3");
    EXPECT_EQ((messages[4][1].get<std::string>()), "Test Message 4");

    // There isn't really a way to verify the timestamp, but
    // it can at least be printed.
    for (const auto& msg : messages)
    {
        std::cout << "Timestamp: " << msg[0] << "\n";
    }

    // Add another message, it should purge the first one
    logger.log("New Message");

    messages = logger.getLogs();
    ASSERT_EQ(messages.size(), logSize);

    // Check the first and last
    EXPECT_EQ((messages[0][1].get<std::string>()), "Test Message 1");
    EXPECT_EQ((messages[4][1].get<std::string>()), "New Message");

    auto path = logger.saveToTempFile();
    ASSERT_TRUE(std::filesystem::exists(path));
    std::filesystem::remove(path);

    logger.clear();
    messages = logger.getLogs();
    EXPECT_TRUE(messages.empty());
}
