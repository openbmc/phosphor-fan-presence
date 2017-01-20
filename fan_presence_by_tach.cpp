#include "config.h"
#include "fan_presence_interface.hpp"

int main(void)
{
    std::string fanPath = "org/openbmc/sensors/tach/fan0H";

    auto tach = phosphor::fan::presence::getTach(
                    fanPath,
                    FAN_SENSOR_VALUE_INTERF);
    if (tach > 0)
    {
        printf("Fan present with tach = %u",tach);
    }

    return 0;
}
