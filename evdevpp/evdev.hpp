#pragma once

#include <fcntl.h>
#include <libevdev/libevdev.h>
#include <memory>
#include <string>
#include <unistd.h>
#include <tuple>

namespace evdevpp
{
namespace evdev
{

using EvDevPtr = libevdev*;

namespace details
{

/** @brief unique_ptr functor to release an evdev reference. */
struct EvDevDeleter
{
    void operator()(libevdev* ptr) const
    {
        deleter(ptr);
    }

    decltype(&libevdev_free) deleter = libevdev_free;
};

/** @brief unique_ptr functor to close a file descriptor. */
struct EvFdCloser
{
    void operator()(int* ptr) const
    {
        deleter(*ptr);
    }

    decltype(&close) deleter = close;
};

/* @brief Alias evdev to a unique_ptr type for auto-release. */
using EvDev = std::unique_ptr<libevdev, EvDevDeleter>;

/* @brief Alias file descriptor to a unique_ptr type for auto-close. */
using EvFd = std::unique_ptr<int, EvFdCloser>;

} // namespace details

/** @class Event
 *  @brief Provides C++ bindings to the libevdev C API.
 */
class EvDev
{
    public:
        /* Define all of the basic class operations:
         *     Not allowed:
         *         - Default constructor to avoid nullptrs.
         *         - Copy operations due to internal unique_ptr.
         *     Allowed:
         *         - Move operations.
         *         - Destructor.
         */
        EvDev() = delete;
        EvDev(const EvDev&) = delete;
        EvDev& operator=(const EvDev&) = delete;
        EvDev(EvDev&&) = default;
        EvDev& operator=(EvDev&&) = default;
        ~EvDev() = default;

        /** @brief New evdev. */
        explicit EvDev(
                const std::string& device) :
            evdev(nullptr), evfd(nullptr)
        {
            auto fd = open(device.c_str(), O_RDONLY | O_NONBLOCK);
            EvDevPtr dev;
            libevdev_new_from_fd(fd, &dev);
            evdev.reset(dev);
            evfd.reset(&fd);
        }

        /** @brief Provide the file descriptor. */
        auto fd()
        {
            return *evfd;
        }

        /** @brief Get the current event state. */
        auto fetch(unsigned int type, unsigned int code)
        {
            int val;
            libevdev_fetch_event_value(evdev.get(), type, code, &val);
            return val;
        }

        /** @brief Get the next event. */
        auto next()
        {
            struct input_event ev;
            while (true)
            {
                libevdev_next_event(
                        evdev.get(), LIBEVDEV_READ_FLAG_NORMAL, &ev);
                if (ev.type == EV_SYN && ev.code == SYN_REPORT)
                    continue;

                break;
            }
            return std::make_tuple(ev.type, ev.code, ev.value);
        }

    private:

        EvDevPtr get()
        {
            return evdev.get();
        }

        details::EvDev evdev;
        details::EvFd evfd;
};

} // namespace evdev
} // namespace evdevpp
