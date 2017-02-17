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

void FanEnclosure::addSensor(
    std::unique_ptr<Sensor>&& sensor)
{
    FanEnclosure::sensors.push_back(std::move(sensor));
}

} // namespace presence
} // namespace fan
} // namespace phosphor
