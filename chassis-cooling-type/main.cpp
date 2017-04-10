#include <iostream>
#include <memory>
#include <sdbusplus/bus.hpp>
#include <phosphor-logging/log.hpp>
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
                    auto gpiocode = std::stoul(keycode);
                    coolingType.readGpio(gpiopath, gpiocode);
                }
                else
                {
                    std::cerr << "\n--event=<keycode> argument required\n";
                }
            }

            coolingType.updateInventory();
            rc = 0;
        }

        catch (std::exception& err)
        {
            rc = -1;
            phosphor::logging::log<phosphor::logging::level::ERR>(err.what());
        }

    }

    return rc;
}

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
