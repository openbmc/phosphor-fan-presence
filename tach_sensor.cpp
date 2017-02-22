#include <sdbusplus/exception.hpp>
#include <phosphor-logging/log.hpp>
#include "tach_sensor.hpp"
#include "fan_enclosure.hpp"


namespace phosphor
{
namespace fan
{
namespace presence
{

using namespace phosphor::logging;

bool TachSensor::isPresent()
{
    return (tach != 0);
}

// Tach signal callback handler
int TachSensor::handleTachChangeSignal(sd_bus_message* msg,
                                       void* usrData,
                                       sd_bus_error* err)
{
    auto sdbpMsg = sdbusplus::message::message(msg);
    static_cast<TachSensor*>(usrData)->handleTachChange(sdbpMsg, err);
    return 0;
}

void TachSensor::handleTachChange(sdbusplus::message::message& sdbpMsg,
                                  sd_bus_error* err)
{
    std::string msgSensor;
    std::map<std::string, sdbusplus::message::variant<int64_t>> msgData;
    sdbpMsg.read(msgSensor, msgData);
    // Find interface with value property
    if (msgSensor.compare("xyz.openbmc_project.Sensor.Value") == 0)
    {
        // Find the 'Value' property containing tach
        auto valPropMap = msgData.find("Value");
        if (valPropMap != msgData.end())
        {
            tach = sdbusplus::message::variant_ns::get<int64_t>(
                valPropMap->second);
        }
    }
    // Update inventory according to latest tach reported
    fanEnc->updInventory();
}

} // namespace presence
} // namespace fan
} // namespace phosphor
