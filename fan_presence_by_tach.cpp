#include "config.h"
#include "fan_presence_interface.hpp"

int main(void)
{
    //TODO Read fan-presence-detect-by-tach conf file for list of fans
    std::string fanPath = "org/openbmc/sensors/tach/fan0H";

    // Get the tach for each fan listed
    auto tach = phosphor::fan::presence::getTach(
                    fanPath,
                    FAN_SENSOR_VALUE_IFACE);

    //TODO Update inventory with fan presence when tach > 0
    if (tach > 0)
    {
        printf("Fan present with tach = %u",tach);
    }

    //TODO Monitor for fan presence changes when powered on(daemon?)
    return 0;
}
