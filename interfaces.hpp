#pragma once

#include <string>

namespace phosphor
{
namespace fan
{
namespace presence
{

    int64_t getTach(const std::string& objpath, const std::string& iface);

} // namespace presence
} // namespace fan
} // namespace phosphor
