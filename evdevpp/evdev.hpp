#pragma once

#include <fcntl.h>
#include <libevdev/libevdev.h>
#include <memory>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <string>
#include <tuple>
#include <unistd.h>
#include <xyz/openbmc_project/Common/error.hpp>

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

/* @brief Alias evdev to a unique_ptr type for auto-release. */
using EvDev = std::unique_ptr<libevdev, EvDevDeleter>;

} // namespace details

/** @class EvDev
 *  @brief Provides C++ bindings to the libevdev C API.
 */
class EvDev
{
    private:
        using InternalFailure = sdbusplus::xyz::openbmc_project::Common::
            Error::InternalFailure;

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

        /** @brief Conversion constructor from evdev. */
        explicit EvDev(EvDevPtr ptr) : evdev(ptr) {}

        /** @brief Get the current event state. */
        auto fetch(unsigned int type, unsigned int code)
        {
            int val;
            auto rc = libevdev_fetch_event_value(
                    evdev.get(), type, code, &val);
            if (!rc)
            {
                phosphor::logging::elog<InternalFailure>();
            }

            return val;
        }

        /** @brief Get the next event. */
        auto next()
        {
            struct input_event ev;
            while (true)
            {
                auto rc = libevdev_next_event(
                        evdev.get(), LIBEVDEV_READ_FLAG_NORMAL, &ev);
                if (rc < 0)
                {
                    phosphor::logging::elog<InternalFailure>();
                }

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
};

inline auto newFromFD(int fd)
{
    using InternalFailure = sdbusplus::xyz::openbmc_project::Common::
        Error::InternalFailure;

    EvDevPtr dev = nullptr;
    auto rc = libevdev_new_from_fd(fd, &dev);

    if (rc)
    {
        phosphor::logging::elog<InternalFailure>();
    }

    return EvDev(dev);
}
} // namespace evdev
} // namespace evdevpp
