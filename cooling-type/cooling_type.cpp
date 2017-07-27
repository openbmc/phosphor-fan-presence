#include <fcntl.h>
#include <unistd.h>
#include <sdbusplus/bus.hpp>
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <libevdev/libevdev.h>
#include "utility.hpp"
#include "cooling_type.hpp"

namespace phosphor
{
namespace cooling
{
namespace type
{

// For throwing exception
using namespace phosphor::logging;
using InternalFailure = sdbusplus::xyz::openbmc_project::Common::
                            Error::InternalFailure;

std::unique_ptr<libevdev, FreeEvDev>  evdevOpen(int fd)
{
    libevdev* gpioDev = nullptr;

    auto rc = libevdev_new_from_fd(fd, &gpioDev);
    if (!rc)
    {
        return decltype(evdevOpen(0))(gpioDev);
    }

    log<level::ERR>("Failed to get libevdev from file descriptor",
             entry("RC=%d", rc));
    elog<InternalFailure>();
    return decltype(evdevOpen(0))(nullptr);
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

    gpioFd.open(gpioPath.c_str(), O_RDONLY);

    auto gpioDev = evdevOpen(gpioFd());

    auto value = static_cast<int>(0);
    auto fetch_rc = libevdev_fetch_event_value(gpioDev.get(), EV_KEY,
                                               keycode, &value);
    if (0 == fetch_rc)
    {
       log<level::ERR>("Device does not support event type",
            entry("KEYCODE=%d", keycode));
       elog<InternalFailure>();
    }

    // TODO openbmc/phosphor-fan-presence#6
    if (value > 0)
    {
        setAirCooled();
    }
    else
    {
        setWaterCooled();
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
        log<level::ERR>(
            "Error in inventory manager call to update inventory");
        elog<InternalFailure>();
    }
}

}
}
}
// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
