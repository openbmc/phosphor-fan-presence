#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include "fcntl.h"
#include "unistd.h"
#include <sdbusplus/bus.hpp>
#include <phosphor-logging/log.hpp>
#include "utility.hpp"
#include <libevdev/libevdev.h>
#include "argument.hpp"
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

void CoolingType::setupGpio(std::string gpioPath)
{
    using namespace phosphor::logging;

    gpioFd = open(gpioPath.c_str(), O_RDONLY);
    if (gpioFd < 0)
    {
        log<level::ERR>("Failed to open GPIO file device",
                        entry("%s", gpioPath.c_str()));
    }
    else
    {
        int rc = libevdev_new_from_fd(gpioFd, &gpioDev);
        if (rc < 0)
        {
            fprintf(stderr, "Failed to get file descriptor for %s (%s)\n", gpioPath.c_str(),
                    strerror(-rc));
            log<level::ERR>("Failed to get file descriptor for ",
                            entry("path=%s strerror(%s)\n", gpioPath.c_str(),
                                  strerror(-rc)));
        }
        else
        {
            // Used to descript state changes of ... key-like devices.
            unsigned int evType = EV_KEY;
            // ... describe stateful binary switches.
            unsigned int evCode = EV_SW;
            int value = 1;
            //int value = libevdev_get_event_value(&gpioDev, evType, evCode);
            int fetch_rc = libevdev_fetch_event_value (gpioDev, evType, evCode,
                                                       &value);
            if (0 ==fetch_rc)
            {
                log<level::ERR>("Device does not support event type and code",
                                entry("type=%d", evType),
                                entry("code=%d", evCode));
            }

            if (value > 0)
            {
                setAirCooled();
            }
            else
            {
                setWaterCooled();
            }
        }
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

void CoolingType::updateInventory(void)
{
    using namespace phosphor::logging;

    ObjectMap invObj = getObjectMap();

    std::string invService;
    try
    {
        invService = phosphor::fan::presence::getInvService(bus);
    }
    catch (const std::runtime_error& err)
    {
        log<level::ERR>(err.what());
        return;
    }

    // Update inventory
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

// Utility function to find the device string for a given pin name.
std::string findGpio(std::string pinName)
{
    std::string path = "/dev/null";
    //TODO
    return path;
}

}
}
}

int main(int argc, char* argv[])
{
    int rc = -1;
    auto options = std::make_unique<ArgumentParser>(argc, argv);

    if (argc < 2)
    {
        std::cerr << std::endl << "Too few arguments" << std::endl;
        options->usage(argv);
    }
    else
    {
        auto bus = sdbusplus::bus::new_default();
        auto coolingType = std::make_unique<phosphor::chassis::cooling::CoolingType>
                           (bus);

        auto gpiopin = (*options)["gpio"];
        if (gpiopin != ArgumentParser::empty_string)
        {
            auto gpiopath = phosphor::chassis::cooling::findGpio(gpiopin);
            coolingType->setupGpio(gpiopath);
        }

        auto air = (*options)["air"];
        if (air != ArgumentParser::empty_string)
        {
            coolingType->setAirCooled();
        }

        auto water = (*options)["water"];
        if (water != ArgumentParser::empty_string)
        {
            coolingType->setWaterCooled();
        }

        coolingType->updateInventory();

        rc = 0;
    }

    return rc;
}

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
