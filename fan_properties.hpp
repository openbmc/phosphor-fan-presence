#pragma once

#include <string>
#include <vector>
#include <tuple>


namespace phosphor
{
namespace fan
{

using Properties = std::tuple<std::string,
                              std::string,
                              std::vector<std::string>>;

} // namespace fan
} // namespace phosphor
