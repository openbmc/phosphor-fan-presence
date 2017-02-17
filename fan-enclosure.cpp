#include "fan-enclosure.hpp"


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
        bool isPresent = false;

        // Determine if all sensors show fan is not present
        for (auto const& sensor: FanEnclosure::sensors)
        {
            if (sensor->isPresent())
            {
                isPresent = true;
                break;
            }
        }
        if (!isPresent)
        {
            //TODO Update inventory for this fan
        }
    }

    void FanEnclosure::addSensor(
        std::unique_ptr<phosphor::fan::presence::Sensor> sensor)
    {
        FanEnclosure::sensors.emplace_back(std::move(sensor));
    }

} // namespace presence
} // namespace fan
} // namespace phosphor
