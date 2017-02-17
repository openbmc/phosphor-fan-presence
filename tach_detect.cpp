#include <vector>
#include <sdbusplus/bus.hpp>
#include "fan_enclosure.hpp"
#include "fan_detect_defs.hpp"


int main(void)
{
    auto bus = sdbusplus::bus::new_default();

    std::vector<std::shared_ptr<phosphor::fan::presence::FanEnclosure>> fans;

    for (auto const &detectMap: fanDetectMap)
    {
        if (detectMap.first == "tach")
        {
            for (auto const& fanProp: detectMap.second)
            {
                auto fan = std::make_shared<
                    phosphor::fan::presence::FanEnclosure>(bus,
                                                           fanProp);
                // TODO Add sensors to fan object
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
