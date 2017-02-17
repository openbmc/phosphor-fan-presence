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

    void FanEnclosure::addSensor(
        std::unique_ptr<phosphor::fan::presence::Sensor> sensor)
    {
        FanEnclosure::sensors.emplace_back(std::move(sensor));
    }

} // namespace presence
} // namespace fan
} // namespace phosphor
