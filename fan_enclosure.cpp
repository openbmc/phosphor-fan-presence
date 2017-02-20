#include <algorithm>
#include "fan_enclosure.hpp"


namespace phosphor
{
namespace fan
{
namespace presence
{

//TODO Should get these from phosphor-objmgr config.h
constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/ObjectMapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";

//TODO Should get these from phosphor-inventory-manager config.h
constexpr auto INVENTORY_PATH = "/xyz/openbmc_project/inventory";
constexpr auto INVENTORY_INTF = "xyz.openbmc_project.Inventory.Manager";

ObjectMap FanEnclosure::getObjectMap()
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
    invIntf.emplace("xyz.openbmc_project.Inventory.Item", invProp);
    Object fanInvPath = invPath;
    invObj.emplace(fanInvPath, invIntf);

    return invObj;
}

std::string FanEnclosure::getInvService()
{
    auto mapperCall = bus.new_method_call(MAPPER_BUSNAME,
                                          MAPPER_PATH,
                                          MAPPER_INTERFACE,
                                          "GetObject");

    mapperCall.append(INVENTORY_PATH);
    mapperCall.append(std::vector<std::string>({INVENTORY_INTF}));

    auto mapperResponseMsg = bus.call(mapperCall);
    if (mapperResponseMsg.is_method_error())
    {
        //TODO Retry or throw exception/log error?
    }

    std::map<std::string, std::vector<std::string>> mapperResponse;
    mapperResponseMsg.read(mapperResponse);

    if (mapperResponse.begin() == mapperResponse.end())
    {
        //TODO Nothing found, throw exception/log error?
    }

    return mapperResponse.begin()->first;
}

void FanEnclosure::updInventory()
{
    //Get inventory object for this fan
    ObjectMap invObj = getObjectMap();
    //Get inventory manager service name from mapper
    std::string invService = getInvService();
    //TODO Update inventory for this fan
}

void FanEnclosure::addSensor(
    std::unique_ptr<phosphor::fan::presence::Sensor>&& sensor)
{
    FanEnclosure::sensors.push_back(std::move(sensor));
}

} // namespace presence
} // namespace fan
} // namespace phosphor
