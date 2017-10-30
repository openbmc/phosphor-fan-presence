#pragma once

#include "types.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{
namespace match
{

/**
 * @brief A match function that constructs a PropertiesChanged match string
 * @details Constructs a PropertiesChanged match string with a given object
 * path and interface
 *
 * @param[in] obj - Object's path name
 * @param[in] iface - Interface name
 *
 * @return Match lambda function
 *     A Match function of a PropertiesChanged match string
 */
Match propertiesChanged(const std::string& obj, const std::string& iface);

/**
 * @brief A match function that constructs an InterfacesAdded match string
 * @details Constructs an InterfacesAdded match string with a given object
 * path
 *
 * @param[in] obj - Object's path name
 *
 * @return Match lambda function
 *     A Match function of a InterfacesAdded match string
 */
Match interfacesAdded(const std::string& obj);

/**
 * @brief A match function that constructs a NameOwnerChanged match string
 * @details Constructs a NameOwnerChanged match string with a given object
 * path and interface
 *
 * @param[in] obj - Object's path name
 * @param[in] iface - Interface name
 *
 * @return Match lambda function
 *     A Match function of a NameOwnerChanged match string
 */
Match nameOwnerChanged(const std::string& obj, const std::string& iface);

} // namespace match
} // namespace control
} // namespace fan
} // namespace phosphor
