#include <algorithm>
#include "fan_enclosure.hpp"


namespace phosphor
{
namespace fan
{
namespace presence
{

void FanEnclosure::addInventory()
{
    //TODO Add this fan to inventory
}

    void FanEnclosure::updInventory()
    {
        auto presPred = [](auto const& s) {return s->isPresent();};
        // Determine if all sensors show fan is not present
        auto isPresent = std::any_of(FanEnclosure::sensors.begin(),
                                     FanEnclosure::sensors.end(),
                                     presPred);
        if (!isPresent)
        {
            //TODO Update inventory for this fan
        }
    }

    void FanEnclosure::addSensor(
        std::unique_ptr<phosphor::fan::presence::Sensor>&& sensor)
    {
        FanEnclosure::sensors.push_back(std::move(sensor));
    }

} // namespace presence
} // namespace fan
} // namespace phosphor
