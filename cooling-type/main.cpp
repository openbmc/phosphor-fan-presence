#include "cooling_type.hpp"
#include "sdbusplus.hpp"

#include <CLI/CLI.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

#include <format>
#include <iostream>
#include <memory>

using namespace phosphor::cooling::type;
using namespace phosphor::fan::util;
using namespace phosphor::logging;

int main(int argc, char* argv[])
{
    auto rc = 1;

    CLI::App app{"Phosphor Cooling type"};
    std::string objpath{};
    bool air = false;
    bool water = false;
    std::string gpiopath{};
    std::string keycode{};

    app.add_option("-p,--path", objpath,
                   "object path under inventory to have CoolingType updated")
        ->required();
    app.add_flag("-a,--air", air,
                 "Force 'AirCooled' property to be set to true.");
    app.add_flag("-w,--water", water,
                 "Force 'WaterCooled' property to be set to true.");
    app.add_option(
        "-d,--dev", gpiopath,
        "Device to read for GPIO pin state to determine 'WaterCooled' (true) and 'AirCooled' (false)");
    app.add_option("-e,--event", keycode, "Keycode for pin to read");
    app.allow_extras();
    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::Error& e)
    {
        return app.exit(e);
    }

    if (objpath.empty())
    {
        log<level::ERR>("Bus path argument required");
        return rc;
    }

    auto bus = sdbusplus::bus::new_default();
    CoolingType coolingType(bus);

    try
    {
        if (air)
        {
            coolingType.setAirCooled();
        }

        if (water)
        {
            coolingType.setWaterCooled();
        }

        if (!gpiopath.empty())
        {
            if (keycode.empty())
            {
                log<level::ERR>("--event=<keycode> argument required\n");
                return rc;
            }
            auto gpiocode = std::stoul(keycode);
            coolingType.readGpio(gpiopath, gpiocode);
        }

        coolingType.updateInventory(objpath);
        rc = 0;
    }
    catch (const DBusMethodError& dme)
    {
        log<level::ERR>(std::format("Uncaught DBus method failure exception "
                                    "Busname: {} "
                                    "Path: {} "
                                    "Interface: {} "
                                    "Method: {}",
                                    dme.busName, dme.path, dme.interface,
                                    dme.method)
                            .c_str());
    }
    catch (const std::exception& err)
    {
        log<phosphor::logging::level::ERR>(err.what());
    }

    return rc;
}

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
