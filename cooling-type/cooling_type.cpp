#include <unistd.h>
#include <fcntl.h>
#include <sdbusplus/bus.hpp>
#include <phosphor-logging/log.hpp>
#include <utility.hpp>
#include "cooling_type.hpp"

namespace phosphor
{
namespace cooling
{
namespace type
{

void CoolingType::setAirCooled()
{
    airCooled = true;
}

void CoolingType::setWaterCooled()
{
    waterCooled = true;
}

void CoolingType::setupGpio(const std::string& gpioPath)
{
    using namespace phosphor::logging;

    gpioFd = open(gpioPath.c_str(), O_RDONLY);
    if (gpioFd.is_open())
    {
        auto rc = 0;//libevdev_new_from_fd(gpiofd, &gpioDev);//FIXME
        if (rc < 0)
        {
            log<level::ERR>("Failed to get file descriptor for ",
                            entry("path=%s strerror(%s)\n", gpioPath.c_str(),
                                  strerror(-rc)));
        }

        //TODO - more to go here?
    }
    else
    {
        perror("Failed to open GPIO file device");
    }

}

CoolingType::ObjectMap CoolingType::getObjectMap(const std::string& objpath)
{
    ObjectMap invObj;
    InterfaceMap invIntf;
    PropertyMap invProp;

    invProp.emplace("AirCooled", airCooled);
    invProp.emplace("WaterCooled", waterCooled);
    invIntf.emplace("xyz.openbmc_project.Inventory.Decorator.CoolingType",
                    std::move(invProp));
    invObj.emplace(objpath, std::move(invIntf));

    return invObj;
}

void CoolingType::updateInventory(const std::string& objpath)
{
    using namespace phosphor::logging;

    ObjectMap invObj = getObjectMap(objpath);

    std::string invService;

    invService = phosphor::fan::util::getInvService(bus);

    // Update inventory
    auto invMsg = bus.new_method_call(invService.c_str(),
                                      INVENTORY_PATH,
                                      INVENTORY_INTF,
                                      "Notify");
    invMsg.append(std::move(invObj));
    auto invMgrResponseMsg = bus.call(invMsg);
    if (invMgrResponseMsg.is_method_error())
    {
        throw std::runtime_error(
            "Error in inventory manager call to update inventory");
    }
}

}
}
}

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
