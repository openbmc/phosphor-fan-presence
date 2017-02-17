#include <vector>
#include <sdbusplus/bus.hpp>
#include "fan_enclosure.hpp"
#include "fan_detect_defs.hpp"
#include "tach_sensor.hpp"


int main(void)
{
    auto bus = sdbusplus::bus::new_default();

    std::vector<std::unique_ptr<phosphor::fan::presence::FanEnclosure>> fans;

    for (auto const& detectMap: fanDetectMap)
    {
        if (detectMap.first == "tach")
        {
            for (auto const& fanProp: detectMap.second)
            {
                auto fan = std::make_unique<
                    phosphor::fan::presence::FanEnclosure>(bus,
                                                           fanProp);
                for (auto const &fanSensor: std::get<2>(fanProp))
                {
                    auto sensor = std::make_unique<
                        phosphor::fan::presence::TachSensor>(bus,
                                                             fanSensor,
                                                             *fan);
                    fan->addSensor(std::move(sensor));
                }
                fans.push_back(std::move(fan));
            }
        }
    }

    while (true)
    {
        // Respond to dbus signals
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
