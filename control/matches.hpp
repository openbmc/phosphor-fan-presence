#pragma once

#include <sdbusplus/bus.hpp>
#include "sdbusplus.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{
namespace match
{

using namespace phosphor::fan;
using namespace sdbusplus::bus::match;
using InternalFailure = sdbusplus::xyz::openbmc_project::Common::
                             Error::InternalFailure;

/**
 * @brief A match function that constructs a PropertiesChanged match string
 * @details Constructs a PropertiesChanged match string with a given object
 * path and interface
 *
 * @param[in] obj - Object's path name
 * @param[in] iface - Interface name
 *
 * @return - A PropertiesChanged match string
 */
inline auto propertiesChanged(const std::string& obj, const std::string& iface)
{
    return rules::propertiesChanged(obj, iface);
}

/**
 * @brief A match function that constructs an InterfacesAdded match string
 * @details Constructs an InterfacesAdded match string with a given object
 * path
 *
 * @param[in] obj - Object's path name
 *
 * @return - An InterfacesAdded match string
 */
inline auto interfacesAdded(const std::string& obj)
{
    return rules::interfacesAdded(obj);
}

/**
 * @brief A match function that constructs a NameOwnerChanged match string
 * @details Constructs a NameOwnerChanged match string with a given object
 * path and interface
 *
 * @param[in] obj - Object's path name
 * @param[in] iface - Interface name
 *
 * @return - A NameOwnerChanged match string
 */
inline auto nameOwnerChanged(const std::string& obj, const std::string& iface)
{
    std::string noc;
    try
    {
        noc = rules::nameOwnerChanged(util::SDBusPlus::getService(obj, iface));
    }
    catch (const InternalFailure& ife)
    {
        // Unable to construct NameOwnerChanged match string
    }
    return noc;
}

} // namespace match
} // namespace control
} // namespace fan
} // namespace phosphor
