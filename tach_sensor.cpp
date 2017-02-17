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
        return (tach != 0);
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
            std::map<std::string, std::map<std::string,
                sdbusplus::message::variant<int64_t>>> msgData;
            auto sdbpMsg = sdbusplus::message::message(msg);
            sdbpMsg.read(msgData);
            // Find interface with value property
            auto valIntfMap = msgData.find("xyz.openbmc_project.Sensor.Value");
            if (valIntfMap != msgData.end())
            {
                // Find the 'Value' property containing tach
                auto valPropMap = valIntfMap->second.find("Value");
                if (valPropMap != valIntfMap->second.end())
                {
                    tach = sdbusplus::message::variant_ns::get<int64_t>(
                        valPropMap->second);
                }
            }
            // Update inventory according to latest tach reported
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
