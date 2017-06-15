#include <gtest/gtest.h>
#include <thread>
#include <unistd.h>
#include "sdevent/event.hpp"
#include "sdevent/io.hpp"

TEST(IoTest, TestIo)
{
    auto loop = sdevent::event::newDefault();
    auto expected = 100;
    volatile auto actual = 0;

    std::array<int, 2> fds;
    pipe(fds.data());

    auto t = std::thread([&loop](){loop.loop();});

    sdevent::event::io::Io io(
        loop,
        fds.data()[0],
        [&fds, &actual, &loop](auto& s)
        {
            auto tmp = 0;
            read(fds.data()[0], &tmp, sizeof(tmp));
            actual = tmp;
            loop.exit();
        });

    write(fds.data()[1], &expected, sizeof(expected));
    t.join();
    close(fds.data()[0]);
    close(fds.data()[1]);

    ASSERT_EQ(expected, actual);
}
