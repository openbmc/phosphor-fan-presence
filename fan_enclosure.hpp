#pragma once

#include <sdbusplus/bus.hpp>
#include "fan_properties.hpp"
#include "sensor_base.hpp"


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
                        invPath(fanProp.inv),
                        fanDesc(fanProp.desc)
        {
            //Add this fan to inventory
            addInventory();
        }

        void addSensor(
            std::unique_ptr<phosphor::fan::presence::Sensor>&& sensor);

    private:
        sdbusplus::bus::bus& bus;
        const std::string invPath;
        const std::string fanDesc;
        std::vector<std::unique_ptr<phosphor::fan::presence::Sensor>> sensors;

        void addInventory();

};

} // namespace presence
} // namespace fan
} // namespace phosphor
