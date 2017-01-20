#include "config.h"
#include "interfaces.hpp"

int main(void)
{
    //TODO Read fan-presence-detect-by-tach conf file for list of fans
    std::string fanPath = "xyz/openbmc_project/sensors/fan_tach/blower1";

    // Get the tach for each fan listed
    auto tach = phosphor::fan::presence::getTach(
                    fanPath,
                    FAN_SENSOR_VALUE_IFACE);

    //TODO Create/update inventory with fan presence when tach > 0
    if (tach > 0)
    {
        fprintf(stdout, "Fan present with tach = %llu\n", tach);
    }

    //TODO Monitor for fan presence changes when powered on(daemon?)
    return 0;
}
