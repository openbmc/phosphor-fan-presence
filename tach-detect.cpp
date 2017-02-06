#include "config.h"
#include "fan-detect-defs.hpp"
#include "tach-handler.hpp"

int main(void)
{
    auto bus = sdbusplus::bus::new_default();

    std::vector<std::unique_ptr<phosphor::fan::presence::Rotor>> rotors;

    //TODO Read fan-presence-detect-by-tach conf file for list of fans
    std::string fanPath = "xyz/openbmc_project/sensors/fan_tach/blower1";
    std::string match = "type='signal',"
                        "interface='org.freedesktop.DBus.Properties',"
                        "member='PropertiesChanged',"
                        "path='"+fanPath+"'";
    rotors.emplace_back(std::make_unique<phosphor::fan::presence::Rotor>(
        bus, match
    ));

    //TODO Create reference for each fan's signal
    /*for (auto &fan: fanDetectionMap)
    {
        rotors.emplace_back(std::make_unique<phosphor::fan::presence::Rotor>(
            bus, match
        ));
    }*/

    while (true) //TODO While powered on only
    {
        // Respond to dbus signals
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
