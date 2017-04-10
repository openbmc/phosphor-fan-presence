#pragma once

#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace fan
{
namespace util
{

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
