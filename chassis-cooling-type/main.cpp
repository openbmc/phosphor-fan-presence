#include <iostream>
#include <memory>
#include <sdbusplus/bus.hpp>
#include "argument.hpp"
#include "cooling_type.hpp"

using namespace phosphor::chassis::cooling;

// Utility function to find the device string for a given pin name.
std::string findGpio(std::string pinName)
{
    std::string path = "/dev/null";
    //TODO
    return path;
}

int main(int argc, char* argv[])
{
    auto rc = -1;
    auto options = ArgumentParser(argc, argv);

    if (argc < 2)
    {
        std::cerr << std::endl << "Too few arguments" << std::endl;
        options->usage(argv);
    }
    else
    {
        auto bus = sdbusplus::bus::new_default();
        phosphor::chassis::cooling::CoolingType coolingType(bus);

        auto gpiopin = (options)["gpio"];
        if (gpiopin != ArgumentParser::empty_string)
        {
            auto gpiopath = findGpio(gpiopin);
            coolingType.setupGpio(gpiopath);
        }

        auto air = (options)["air"];
        if (air != ArgumentParser::empty_string)
        {
            coolingType.setAirCooled();
        }

        auto water = (options)["water"];
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
