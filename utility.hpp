#pragma once

#include <sdbusplus/bus.hpp>
#include <unistd.h>
#include <fcntl.h>
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <xyz/openbmc_project/Common/error.hpp>


using namespace phosphor::logging;
using InternalFailure = sdbusplus::xyz::openbmc_project::Common::
                            Error::InternalFailure;

namespace phosphor
{
namespace fan
{
namespace util
{

class FileDescriptor
{
    public:
        FileDescriptor() = delete;
        FileDescriptor(const FileDescriptor&) = delete;
        FileDescriptor(FileDescriptor&&) = default;
        FileDescriptor& operator=(const FileDescriptor&) = delete;
        FileDescriptor& operator=(FileDescriptor&&) = default;

        FileDescriptor(int fd) : fd(fd)
        {
        }

        ~FileDescriptor()
        {
            if (fd != -1)
            {
                close(fd);
            }
        }

        int operator()()
        {
            return fd;
        }

        void open(const std::string& pathname, int flags)
        {
            fd = ::open(pathname.c_str(), flags);
            if (-1 == fd)
            {
                log<level::ERR>(
                     "Failed to open file device: ",
                     entry("PATHNAME=%s", pathname.c_str()));
                elog<InternalFailure>();
            }
        }

        bool is_open()
        {
            return fd != -1;
        }

    private:
        int fd = -1;

};

/**
 * @brief Get the inventory service name from the mapper object
 *
 * @return The inventory manager service name
 */
std::string getInvService(sdbusplus::bus::bus& bus);


/**
 * @brief Get the service name from the mapper for the
 *        interface and path passed in.
 *
 * @param[in] path - the dbus path name
 * @param[in] interface - the dbus interface name
 * @param[in] bus - the dbus object
 *
 * @return The service name
 */
std::string getService(const std::string& path,
                       const std::string& interface,
                       sdbusplus::bus::bus& bus);

constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

/**
 * Get the current value of the D-Bus property under the specified path
 * and interface.
 *
 * @param[in] bus          - The D-Bus bus object
 * @param[in] path         - The D-Bus path
 * @param[in] interface    - The D-Bus interface
 * @param[in] propertyName - The D-Bus property
 * @param[out] value       - The D-Bus property's value
 */
template <typename T>
void getProperty(sdbusplus::bus::bus& bus,
                 const std::string& path,
                 const std::string& interface,
                 const std::string& propertyName,
                 T& value)
{
    sdbusplus::message::variant<T> property;
    std::string service = phosphor::fan::util::getService(path, interface, bus);

    auto method = bus.new_method_call(service.c_str(),
                                      path.c_str(),
                                      PROPERTY_INTERFACE,
                                      "Get");

    method.append(interface, propertyName);
    auto reply = bus.call(method);

    if (reply.is_method_error())
    {
        log<level::ERR>("Error in call response for retrieving property");
        elog<InternalFailure>();
    }
    reply.read(property);
    value = sdbusplus::message::variant_ns::get<T>(property);
}
}
}
}
