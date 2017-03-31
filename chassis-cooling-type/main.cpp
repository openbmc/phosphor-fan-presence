#include <iostream>
#include <memory> //make_unique
#include <sdbusplus/bus.hpp>
#include "argument.hpp"
#include "cooling_type.hpp"

// Utility function to find the device string for a given pin name.
std::string findGpio(std::string pinName)
{
    std::string path = "/dev/null";
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
        phosphor::chassis::cooling::CoolingType coolingType(bus);

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
