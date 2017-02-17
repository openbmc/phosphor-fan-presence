#include <vector>
#include <sdbusplus/bus.hpp>
#include "fan_enclosure.hpp"
#include "fan_detect_defs.hpp"
#include "tach_sensor.hpp"


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
                for (auto const &fanSensor: std::get<2>(fanProp))
                {
                    std::string match =
                        "type='signal',"
                        "interface='org.freedesktop.DBus.Properties',"
                        "member='PropertiesChanged',"
                        "path='/xyz/openbmc_project/sensors/fan_tach/" +
                            fanSensor + "'";
                    auto sensor = std::make_unique<
                        phosphor::fan::presence::TachSensor>(bus,
                                                             match,
                                                             fan);
                    fan->addSensor(std::move(sensor));
                }
                fans.push_back(fan);
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
