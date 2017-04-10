#include <fcntl.h>
#include <unistd.h>
#include <sdbusplus/bus.hpp>
#include <phosphor-logging/log.hpp>
#include "utility.hpp"
#include <libevdev/libevdev.h>
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
    if (gpioDev != nullptr)
    {
        libevdev_free(gpioDev);
    }

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

void CoolingType::readGpio(const std::string& gpioPath, unsigned int keycode)
{
    using namespace phosphor::logging;

    gpioFd = open(gpioPath.c_str(), O_RDONLY);
    if (gpioFd < 0)
    {
        //TODO - Create error log for failure. openbmc/openbmc#???
        throw std::runtime_error(
            "Failed to open GPIO file device: " + gpioPath);
    }

    auto rc = libevdev_new_from_fd(gpioFd, &gpioDev);
    if (rc < 0)
    {
        //TODO - Create error log for failure. openbmc/openbmc#???
        throw std::runtime_error(
            "Failed to get file descriptor for path=" + gpioPath + "strerror("
            + strerror(-rc) + ")");
    }

    auto value = 1;
    auto fetch_rc = libevdev_fetch_event_value(gpioDev, EV_KEY, keycode,
                    &value);
    if (0 == fetch_rc)
    {
        //TODO - Create error log for failure. openbmc/openbmc#???
        throw std::runtime_error(
            "Device does not support event type=EV_KEY and code=" + std::to_string(keycode));
    }

    // TODO openbmc/phosphor-fan-presence#6
    if (value > 0)
    {
        log<level::INFO>("system is water cooled");
        setWaterCooled();
    }
    else
    {
        log<level::INFO>("system is air cooled");
        setAirCooled();
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
    Object invPath("/system/chassis");
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
