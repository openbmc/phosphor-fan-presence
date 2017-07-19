#pragma once

#include <memory>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <systemd/sd-event.h>
#include <xyz/openbmc_project/Common/error.hpp>

namespace sdevent
{
namespace source
{

using SourcePtr = sd_event_source*;

namespace details
{

/** @brief unique_ptr functor to release a source reference. */
struct SourceDeleter
{
    void operator()(sd_event_source* ptr) const
    {
        deleter(ptr);
    }

    decltype(&sd_event_source_unref) deleter = sd_event_source_unref;
};

/* @brief Alias 'source' to a unique_ptr type for auto-release. */
using source = std::unique_ptr<sd_event_source, SourceDeleter>;

} // namespace details

/** @class Source
 *  @brief Provides C++ bindings to the sd_event_source* functions.
 */
class Source
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
        Source() = delete;
        Source(const Source&) = delete;
        Source& operator=(const Source&) = delete;
        Source(Source&&) = default;
        Source& operator=(Source&&) = default;
        ~Source() = default;

        /** @brief Conversion constructor from 'SourcePtr'.
         *
         *  Increments ref-count of the source-pointer and releases it
         *  when done.
         */
        explicit Source(SourcePtr s) : src(sd_event_source_ref(s)) {}

        /** @brief Constructor for 'source'.
         *
         *  Takes ownership of the source-pointer and releases it when done.
         */
        Source(SourcePtr s, std::false_type) : src(s) {}

        /** @brief Check if source contains a real pointer. (non-nullptr). */
        explicit operator bool() const
        {
            return bool(src);
        }

        /** @brief Test whether or not the source can generate events. */
        auto enabled()
        {
            int enabled;
            auto rc = sd_event_source_get_enabled(src.get(), &enabled);
            if (rc < 0)
            {
                phosphor::logging::elog<InternalFailure>();
            }

            return enabled;
        }

        /** @brief Allow the source to generate events. */
        void enable(int enable)
        {
            auto rc = sd_event_source_set_enabled(src.get(), enable);
            if (rc < 0)
            {
                phosphor::logging::elog<InternalFailure>();
            }
        }

    private:
        details::source src;
};
} // namespace source
} // namespace sdevent
