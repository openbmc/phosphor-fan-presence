#pragma once

#include <sdbusplus/bus.hpp>
#include "fan_properties.hpp"
#include "sensor_base.hpp"


using Property = std::string;
using Value = sdbusplus::message::variant<bool, int64_t, std::string>;
// Association between property and its value
using PropertyMap = std::map<Property, Value>;
using Interface = std::string;
// Association between interface and the dbus property
using InterfaceMap = std::map<Interface, PropertyMap>;
using Object = sdbusplus::message::object_path;
// Association between object and the interface
using ObjectMap = std::map<Object, InterfaceMap>;

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
        FanEnclosure& operator=(FanEnclosure&&) = delete;
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

        void updInventory();
        void addSensor(
            std::unique_ptr<Sensor>&& sensor);

    private:
        sdbusplus::bus::bus& bus;
        const std::string invPath;
        const std::string fanDesc;
        std::vector<std::unique_ptr<Sensor>> sensors;

        void addInventory();
        ObjectMap getObjectMap();

};

} // namespace presence
} // namespace fan
} // namespace phosphor
