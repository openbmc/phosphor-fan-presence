#include "tach_sensor.hpp"


namespace phosphor
{
namespace fan
{
namespace presence
{

bool TachSensor::isPresent()
{
    return (tach != 0);
}

} // namespace presence
} // namespace fan
} // namespace phosphor
