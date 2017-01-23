#include <sdbusplus/bus.hpp>
#include "config.h"
#include "sensor_handler.hpp"

int main(void)
{
    auto bus = sdbusplus::bus::new_default();

    //TODO - dlopen libfan_sensor_handler?

    phosphor::fan::Sensor handler(bus, FAN_SENSOR_INPUTS_PATH);

    bus.request_name(FAN_SENSOR_INPUTS_BUSNAME);

    while(true)
    {
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
