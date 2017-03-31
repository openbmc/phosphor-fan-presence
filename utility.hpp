#pragma once

#include <sdbusplus/bus.hpp>
#include <unistd.h>

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

        bool is_open()
        {
            if (fd != -1)
            {
                return true;
            }
            else
            {
                return false;
            }
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

}
}
}
