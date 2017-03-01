#pragma once

#include <string>
#include <vector>
#include <tuple>


namespace phosphor
{
namespace fan
{

/**
 * @brief Fan enclosure properties
 * @details Contains the inventory path, description and list of sensors
 */
using Properties = std::tuple<std::string,
                              std::string,
                              std::vector<std::string>>;

} // namespace fan
} // namespace phosphor
