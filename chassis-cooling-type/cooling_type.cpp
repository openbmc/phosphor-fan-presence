#include <unistd.h>
#include <fcntl.h>
#include <sdbusplus/bus.hpp>
#include <phosphor-logging/log.hpp>
#include "cooling_type.hpp"

namespace phosphor
{
namespace chassis
{
namespace cooling
{

CoolingType::~CoolingType()
{
//    if (gpioFd > 0)
//    {
//        close(gpioFd);
//    }

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

void CoolingType::updateInventory()
{
    //TODO
    //     setProperty(bus, ..., "AirCooled");
    //     setProperty(bus, ..., "WaterCooled");
}

}
}
}

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
