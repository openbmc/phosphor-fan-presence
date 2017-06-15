#include <gtest/gtest.h>
#include <thread>
#include <unistd.h>
#include "sdevent/event.hpp"
#include "sdevent/io.hpp"

TEST(IoTest, TestIo)
{
    // Validate an sd event io callback can be
    // constructed, added to an event loop, and
    // that the callback is invoked.

    auto loop = sdevent::event::newDefault();
    auto expected = 100;
    volatile auto actual = 0;

    std::array<int, 2> fds;
    auto rc = pipe(fds.data());
    ASSERT_EQ(rc, 0);

    auto t = std::thread([&loop](){loop.loop();});

    sdevent::event::io::IO io(
        loop,
        fds.data()[0],
        [&fds, &actual, &loop](auto& s)
        {
            auto tmp = 0;
            auto rc = read(fds.data()[0], &tmp, sizeof(tmp));
            ASSERT_GT(rc, 0);
            actual = tmp;
            loop.exit();
        });

    rc = write(fds.data()[1], &expected, sizeof(expected));
    ASSERT_GT(rc, 0);
    t.join();
    close(fds.data()[0]);
    close(fds.data()[1]);

    ASSERT_EQ(expected, actual);
}
