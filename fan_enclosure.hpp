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
            updInventory();
        }

        void updInventory();
        void addSensor(
            std::unique_ptr<phosphor::fan::presence::Sensor>&& sensor);

    private:
        sdbusplus::bus::bus& bus;
        const std::string invPath;
        const std::string fanDesc;
        std::vector<std::unique_ptr<phosphor::fan::presence::Sensor>> sensors;
        bool presState = false;

        bool getCurPresState();
        std::string getInvService();
        ObjectMap getObjectMap(const bool& curPresState);

};

} // namespace presence
} // namespace fan
} // namespace phosphor
