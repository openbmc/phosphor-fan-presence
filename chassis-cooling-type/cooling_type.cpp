#include <unistd.h>
#include <fcntl.h>
#include <sdbusplus/bus.hpp>
#include <phosphor-logging/log.hpp>
#include <utility.hpp>
#include "cooling_type.hpp"

namespace phosphor
{
namespace chassis
{
namespace cooling
{

//TODO Should get these from phosphor-inventory-manager config.h
constexpr auto INVENTORY_PATH = "/xyz/openbmc_project/inventory";
constexpr auto INVENTORY_INTF = "xyz.openbmc_project.Inventory.Manager";

CoolingType::~CoolingType()
{
    if (gpioFd > 0)
    {
        close(gpioFd);
    }

}

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
    if (gpioFd < 0)
    {
        perror("Failed to open GPIO file device");
    }
    else
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
}

CoolingType::ObjectMap CoolingType::getObjectMap()
{
    ObjectMap invObj;
    InterfaceMap invIntf;
    PropertyMap invProp;

    invProp.emplace("AirCooled", airCooled);
    invProp.emplace("WaterCooled", waterCooled);
    invIntf.emplace("xyz.openbmc_project.Inventory.Decorator.CoolingType",
                    std::move(invProp));
    Object invPath("/xyz/openbmc_project/inventory/chassis");
    invObj.emplace(std::move(invPath), std::move(invIntf));

    return invObj;
}

void CoolingType::updateInventory()
{
    using namespace phosphor::logging;

    ObjectMap invObj = getObjectMap();

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
