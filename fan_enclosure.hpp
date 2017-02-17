#pragma once

#include <sdbusplus/bus.hpp>
#include "fan_properties.hpp"


namespace phosphor
{
namespace fan
{
namespace presence
{

class FanEnclosure
{
    public:
        FanEnclosure() = delete;
        FanEnclosure(const FanEnclosure&) = delete;
        FanEnclosure(FanEnclosure&&) = default;
        FanEnclosure& operator=(const FanEnclosure&) = delete;
        FanEnclosure& operator=(FanEnclosure&&) = default;
        ~FanEnclosure() = default;

        FanEnclosure(sdbusplus::bus::bus& bus,
                     const phosphor::fan::Properties& fanProp) :
                        bus(bus),
                        invPath(std::get<0>(fanProp)),
                        fanDesc(std::get<1>(fanProp))
        {
            //Add this fan to inventory
            addInventory();
        }

    private:
        sdbusplus::bus::bus& bus;
        const std::string invPath;
        const std::string fanDesc;

        void addInventory();

};

} // namespace presence
} // namespace fan
} // namespace phosphor
