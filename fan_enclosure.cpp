#include <algorithm>
#include <log.hpp> //TODO phosphor-logging/log.hpp
#include "fan_enclosure.hpp"


namespace phosphor
{
namespace fan
{
namespace presence
{

    using namespace phosphor::logging;

    //TODO Should get these from phosphor-objmgr config.h
    constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
    constexpr auto MAPPER_PATH = "/xyz/openbmc_project/ObjectMapper";
    constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";

    //TODO Should get these from phosphor-inventory-manager config.h
    constexpr auto INVENTORY_PATH = "/xyz/openbmc_project/Inventory";
    constexpr auto INVENTORY_INTF = "xyz.openbmc_project.Inventory.Manager";

    ObjectMap FanEnclosure::getObjectMap()
    {
        ObjectMap invObj;
        InterfaceMap invIntf;
        PropertyMap invProp;
        auto presPred = [](auto const& s) {return s->isPresent();};
        // Determine if all sensors show fan is not present
        auto isPresent = std::any_of(FanEnclosure::sensors.begin(),
                                     FanEnclosure::sensors.end(),
                                     presPred);
        //TODO Power off chassis when X-number of fans missing
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
            throw std::runtime_error(
                "Error in mapper call to get inventory service name");
        }

        std::map<std::string, std::vector<std::string>> mapperResponse;
        mapperResponseMsg.read(mapperResponse);

        if (mapperResponse.empty())
        {
            throw std::runtime_error(
                "Error in mapper response for inventory service name");
        }

        return mapperResponse.begin()->first;
    }

    void FanEnclosure::updInventory()
    {
        //Get inventory object for this fan
        ObjectMap invObj = getObjectMap();
        //Get inventory manager service name from mapper
        std::string invService;
        try
        {
            invService = getInvService();
        }
        catch (const std::runtime_error& err)
        {
            log<level::ERR>(err.what());
            return;
        }
        // Update inventory for this fan
        auto invMsg = bus.new_method_call(invService.c_str(),
                                          INVENTORY_PATH,
                                          INVENTORY_INTF,
                                          "Notify");
        invMsg.append(std::move(invObj));
        auto invMgrResponseMsg = bus.call(invMsg);
        if (invMgrResponseMsg.is_method_error())
        {
            log<level::ERR>(
                "Error in inventory manager call to update inventory");
            return;
        }
    }

    void FanEnclosure::addSensor(
        std::unique_ptr<phosphor::fan::presence::Sensor>&& sensor)
    {
        FanEnclosure::sensors.push_back(std::move(sensor));
    }

} // namespace presence
} // namespace fan
} // namespace phosphor
