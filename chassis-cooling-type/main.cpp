#include <iostream>
#include <memory>
#include <sdbusplus/bus.hpp>
#include "argument.hpp"
#include "cooling_type.hpp"

using namespace phosphor::chassis::cooling;

int main(int argc, char* argv[])
{
    auto rc = -1;
    auto options = std::make_unique<ArgumentParser>(argc, argv);

    if (argc < 2)
    {
        std::cerr << std::endl << "Too few arguments" << std::endl;
        options->usage(argv);
    }
    else
    {
        auto bus = sdbusplus::bus::new_default();
        phosphor::chassis::cooling::CoolingType coolingType(bus);

        try
        {
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

            auto gpiopath = (*options)["dev"];
            if (gpiopath != ArgumentParser::empty_string)
            {
                auto keycode = (*options)["event"];
                if (keycode != ArgumentParser::empty_string)
                {
                    auto gpiocode = strtoul(keycode.c_str(), 0, 0);
                    coolingType.setupGpio(gpiopath, gpiocode);
                }
                else
                {
                    std::cerr << std::endl
                              << "--event=<keycode> argument required"
                              << std::endl;
                }
            }

            coolingType.updateInventory();
        }

        catch (int& code)
        {
            std::cerr << std::endl << "(Failed to update chassis cooling type:"
                      << code << ")" << std::endl;
        }

        rc = 0;
    }

    return rc;
}

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
