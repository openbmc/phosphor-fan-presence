#include <algorithm>
#include "conditions.hpp"
#include "sdbusplus.hpp"

namespace phosphor
{
namespace fan
{
namespace monitor
{
namespace condition
{

Condition propertiesMatch(std::vector<PropertyState>&& propStates)
{
    return [pStates = std::move(propStates)](sdbusplus::bus::bus& bus)
    {
        return std::all_of(
            pStates.begin(),
            pStates.end(),
            [&bus](const auto& p)
        {
            return util::SDBusPlus::getPropertyVariant<PropertyValue>(
                bus,
                std::get<propObj>(p.first),
                std::get<propIface>(p.first),
                std::get<propName>(p.first)) == p.second;
        });
    };
}

} // namespace condition
} // namespace monitor
} // namespace fan
} // namespace phosphor
