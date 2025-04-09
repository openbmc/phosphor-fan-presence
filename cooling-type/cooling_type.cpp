#include "cooling_type.hpp"

#include "sdbusplus.hpp"
#include "utility.hpp"

#include <fcntl.h>
#include <libevdev/libevdev.h>
#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace cooling
{
namespace type
{

using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

std::unique_ptr<libevdev, FreeEvDev> evdevOpen(int fd)
{
    libevdev* gpioDev = nullptr;

    auto rc = libevdev_new_from_fd(fd, &gpioDev);
    if (!rc)
    {
        return decltype(evdevOpen(0))(gpioDev);
    }

    lg2::error(
        "Failed to get libevdev from file descriptor {FD}, return code {RC}",
        "FD", fd, "RC", rc);
    phosphor::logging::elog<InternalFailure>();
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
    gpioFd.open(gpioPath.c_str(), O_RDONLY);

    auto gpioDev = evdevOpen(gpioFd());

    int value = 0;
    auto fetch_rc =
        libevdev_fetch_event_value(gpioDev.get(), EV_KEY, keycode, &value);
    if (0 == fetch_rc)
    {
        lg2::error("Device does not support event type keycode {KEYCODE}",
                   "KEYCODE", keycode);
        phosphor::logging::elog<InternalFailure>();
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
    using namespace phosphor::fan;

    ObjectMap invObj = getObjectMap(objpath);

    // Update inventory
    static_cast<void>(util::SDBusPlus::lookupAndCallMethod(
        bus, util::INVENTORY_PATH, util::INVENTORY_INTF, "Notify",
        std::move(invObj)));
}

} // namespace type
} // namespace cooling
} // namespace phosphor
