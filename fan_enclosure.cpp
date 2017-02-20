#include <algorithm>
#include "fan_enclosure.hpp"


namespace phosphor
{
namespace fan
{
namespace presence
{

FanEnclosure::ObjectMap FanEnclosure::getObjectMap()
{
    ObjectMap invObj;
    InterfaceMap invIntf;
    PropertyMap invProp;
    auto presPred = [](auto const& s) {return s->isPresent();};
    // Determine if all sensors show fan is not present
    auto isPresent = std::any_of(sensors.begin(),
                                 sensors.end(),
                                 presPred);
    invProp.emplace("Present", isPresent);
    invProp.emplace("PrettyName", fanDesc);
    invIntf.emplace("xyz.openbmc_project.Inventory.Item", std::move(invProp));
    Object fanInvPath = invPath;
    invObj.emplace(std::move(fanInvPath), std::move(invIntf));

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
    std::unique_ptr<Sensor>&& sensor)
{
    FanEnclosure::sensors.push_back(std::move(sensor));
}

} // namespace presence
} // namespace fan
} // namespace phosphor
