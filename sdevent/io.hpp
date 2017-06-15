#pragma once

#include <functional>
#include <memory>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <systemd/sd-event.h>
#include <xyz/openbmc_project/Common/error.hpp>

#include "sdevent/source.hpp"
#include "sdevent/event.hpp"

namespace sdevent
{
namespace event
{
namespace io
{

/** @class IO
 *  @brief Provides C++ bindings to the sd_event_source* io functions.
 */
class IO
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
        IO() = delete;
        IO(const IO&) = delete;
        IO& operator=(const IO&) = delete;
        IO(IO&&) = default;
        IO& operator=(IO&&) = default;
        ~IO() = default;

        using Callback = std::function<void(source::Source&)>;

        /** @brief Register an io callback.
         *
         *  @param[in] event - The event to register on.
         *  @param[in] fd - The file descriptor to poll.
         *  @param[in] callback - The callback method.
         */
        IO(
            sdevent::event::Event& event,
            int fd,
            Callback callback)
            : src(nullptr),
              cb(std::make_unique<Callback>(std::move(callback)))
        {
            sd_event_source* source = nullptr;
            auto rc = sd_event_add_io(
                event.get(),
                &source,
                fd,
                EPOLLIN,
                callCallback,
                cb.get());
            if (rc < 0)
            {
                phosphor::logging::elog<InternalFailure>();
            }

            src = decltype(src){source, std::false_type()};
        }

        /** @brief Set the IO source enable state. */
        void enable(int enable)
        {
            src.enable(enable);
        }

        /** @brief Query the IO enable state. */
        auto enabled()
        {
            return src.enabled();
        }

    private:
        source::Source src;
        std::unique_ptr<Callback> cb = nullptr;

        static int callCallback(sd_event_source* s, int fd, uint32_t events,
                                void* context)
        {
            source::Source source(s);
            auto c = static_cast<Callback*>(context);
            (*c)(source);

            return 0;
        }
};
} // namespace io
} // namespace event
} // namespace sdevent
