#include <sdbusplus/exception.hpp>
#include "tach_sensor.hpp"
#include "fan_enclosure.hpp"


namespace phosphor
{
namespace fan
{
namespace presence
{

    bool TachSensor::isPresent()
    {
        return false;
    }

    // Tach signal callback handler
    int TachSensor::handleTachChangeSignal(sd_bus_message* msg,
                                void* usrData,
                                sd_bus_error* err)
    {
        return static_cast<TachSensor*>(usrData)->handleTachChange(msg, err);
    }

    int TachSensor::handleTachChange(sd_bus_message* msg, sd_bus_error* err)
    {
        try
        {
            int64_t curTach{};
            auto sdbpMsg = sdbusplus::message::message(msg);
            sdbpMsg.read(curTach);
            tach = curTach;

            fanEnc->updInventory();
        }
        catch (sdbusplus::internal_exception_t& e)
        {
            sd_bus_error_set_const(err, e.name(), e.description());
            return -EINVAL;
        }

        return 0;
    }

} // namespace presence
} // namespace fan
} // namespace phosphor
