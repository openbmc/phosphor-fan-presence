/**
 * Copyright © 2017 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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

    // TODO openbmc/phosphor-fan-presence#5
    //      Update to use 'arg0namespace' match option to reduce dbus traffic
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
    fanEnc.updInventory();
}

} // namespace presence
} // namespace fan
} // namespace phosphor
