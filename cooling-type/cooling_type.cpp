#include <unistd.h>
#include <fcntl.h>
#include <sdbusplus/bus.hpp>
#include <phosphor-logging/log.hpp>
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
            throw std::runtime_error("Failed to get libevdev from " +
                                     gpioPath + " rc = " +
                                     std::to_string(rc));
        }

        //TODO - more to go here?
    }
    else
    {
        throw std::runtime_error("Failed to open GPIO file device");
    }

}

void CoolingType::updateInventory(const std::string& objpath)
{
    //TODO
    //     setProperty(bus, ..., "AirCooled");
    //     setProperty(bus, ..., "WaterCooled");
}

}
}
}

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
