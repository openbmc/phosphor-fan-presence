#include <iostream>
#include "fcntl.h"
#include "unistd.h"
#include <sdbusplus/bus.hpp>
#include "argument.hpp"
#include "cooling_type.hpp"

using namespace phosphor::chassis::cooling;

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

void CoolingType::setupGpio(std::string gpioPath)
{
    gpioFd = open(gpioPath.c_str(), O_RDONLY);
    if (gpioFd < 0)
    {
        perror("Failed to open GPIO file device");
    }
    else
    {
        int rc = 0;//libevdev_new_from_fd(gpiofd, &gpioDev);//FIXME
        if (rc < 0)
        {
            fprintf(stderr, "Failed to get file descriptor for %s (%s)\n", gpioPath.c_str(),
                    strerror(-rc));
        }

        //TODO - more to go here?
    }
}

void CoolingType::updateInventory(void)
{
    //TODO
    //     setProperty(bus, ..., "AirCooled");
    //     setProperty(bus, ..., "WaterCooled");
}

// Utility function to find the device string for a given pin name.
std::string findGpio(std::string pinName)
{
    std::string path="/dev/null";
    //TODO
    return path;
}

int main(int argc, char* argv[])
{
    int rc = -1;
    auto options = std::make_unique<ArgumentParser>(argc, argv);

    if (argc < 2)
    {
        std::cerr << std::endl << "Too few arguments" << std::endl;
    }
    else
    {
        auto bus = sdbusplus::bus::new_default();
        CoolingType coolingType(bus);

        auto gpiopin = (*options)["gpio"];
        if (gpiopin != ArgumentParser::empty_string)
        {
            auto gpiopath = findGpio(gpiopin);
            coolingType.setupGpio(gpiopath);
        }

        auto air = (*options)["air"];
        if (air != ArgumentParser::empty_string)
        {
            coolingType.setAirCooled();
        }

        auto water = (*options)["water"];
        if (water != ArgumentParser::empty_string)
        {
            coolingType.setWaterCooled();
        }

        coolingType.updateInventory();

        rc = 0;
    }

    return rc;
}

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
