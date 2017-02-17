#include <vector>
#include <sdbusplus/bus.hpp>
#include "fan-enclosure.hpp"
#include "fan-detect-defs.hpp"
#include "tach-sensor.hpp"


int main(void)
{
    auto bus = sdbusplus::bus::new_default();

    std::vector<std::shared_ptr<phosphor::fan::presence::FanEnclosure>> fans;

    for (auto const &detectMap: fanDetectMap)
    {
        if (detectMap.first == "tach")
        {
            for (auto const &fanProp: detectMap.second)
            {
                auto fan = std::make_shared<
                    phosphor::fan::presence::FanEnclosure>(bus,
                                                           fanProp);
                for (auto const &fanSensor: fanProp.sensors)
                {
                    auto sensor = std::make_unique<
                        phosphor::fan::presence::TachSensor>(bus,
                                                             fanSensor,
                                                             fan);
                    fan->addSensor(std::move(sensor));
                }
                fans.emplace_back(fan);
            }
        }
    }

    while (true) //TODO Only necessary while powered on
    {
        // Respond to dbus signals
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
