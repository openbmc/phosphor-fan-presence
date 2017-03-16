#pragma once

#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace fan
{
namespace presence
{

/**
 * @brief Get the inventory service name from the mapper object
 *
 * @return The inventory manager service name
 */
std::string getInvService(sdbusplus::bus::bus& bus);

}
}
}
