#include <iostream>
#include <memory> //make_unique
#include <sdbusplus/bus.hpp>
#include <phosphor-logging/log.hpp>
#include "argument.hpp"
#include "cooling_type.hpp"

using namespace phosphor::cooling::type;
using namespace phosphor::logging;

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

    auto objpath = (options)["path"];
    if (argc < 2)
    {
        std::cerr << std::endl << "Too few arguments" << std::endl;
        log<level::ERR>("Too few arguments");
        options.usage(argv);
    }
    else if (objpath == ArgumentParser::empty_string)
    {
        log<level::ERR>("Bus path argument required");
    }
    else
    {
        auto bus = sdbusplus::bus::new_default();
        CoolingType coolingType(bus);

        auto gpiopin = (options)["gpio"];
        if (gpiopin != ArgumentParser::empty_string)
        {
            try
            {
                auto gpiopath = findGpio(gpiopin);
                coolingType.setupGpio(gpiopath);
            }
            catch (std::exception& err)
            {
                rc = -1;
                log<phosphor::logging::level::ERR>(err.what());
            }
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

        coolingType.updateInventory(objpath);

        rc = 0;
    }

    return rc;
}

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
