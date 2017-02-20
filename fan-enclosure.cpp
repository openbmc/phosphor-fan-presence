#include "fan-enclosure.hpp"


namespace phosphor
{
namespace fan
{
namespace presence
{

    ObjectMap FanEnclosure::getObjectMap()
    {
        ObjectMap invObj;
        InterfaceMap invIntf;
        PropertyMap invProp;
        bool isPresent = false;

        // Determine if all sensors show fan is not present
        for (auto const& sensor: FanEnclosure::sensors)
        {
            if (sensor->isPresent())
            {
                isPresent = true;
                break;
            }
        }
        invProp.emplace("Present", isPresent);
        invProp.emplace("PrettyName", fanDesc);
        invIntf.emplace("Item", invProp);
        Object fanInvPath = invPath;
        invObj.emplace(fanInvPath, invIntf);

        return invObj;
    }

    void FanEnclosure::addInventory()
    {
        //TODO Add this fan to inventory
    }

    void FanEnclosure::updInventory()
    {
        //Get inventory object for this fan
        ObjectMap invObj = getObjectMap();
        //TODO Update inventory for this fan
    }

    void FanEnclosure::addSensor(
        std::unique_ptr<phosphor::fan::presence::Sensor> sensor)
    {
        FanEnclosure::sensors.emplace_back(std::move(sensor));
    }

} // namespace presence
} // namespace fan
} // namespace phosphor
