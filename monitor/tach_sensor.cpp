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
#include <phosphor-logging/log.hpp>
#include "fan.hpp"
#include "sdbusplus.hpp"
#include "tach_sensor.hpp"
#include "../utility.hpp"

namespace phosphor
{
namespace fan
{
namespace monitor
{

constexpr auto FAN_SENSOR_CONTROL_INTF = "xyz.openbmc_project.Control.FanSpeed";
constexpr auto FAN_SENSOR_VALUE_INTF = "xyz.openbmc_project.Sensor.Value";
constexpr auto FAN_TARGET_PROPERTY = "Target";
constexpr auto FAN_VALUE_PROPERTY = "Value";


/**
 * @brief Helper function to read a property
 *
 * @param[in] interface - the interface the property is on
 * @param[in] propertName - the name of the property
 * @param[in] path - the dbus path
 * @param[in] bus - the dbus object
 * @param[out] value - filled in with the property value
 */
template<typename T>
static void readProperty(const std::string& interface,
                         const std::string& propertyName,
                         const std::string& path,
                         sdbusplus::bus::bus& bus,
                         T& value)
{
    try
    {
        value = util::SDBusPlus::getProperty<T>(bus,
                                                path,
                                                interface,
                                                propertyName);
    }
    catch (std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
    }
}


TachSensor::TachSensor(sdbusplus::bus::bus& bus,
                       Fan& fan,
                       const std::string& id,
                       bool hasTarget,
                       size_t timeout,
                       phosphor::fan::event::EventPtr& events) :
    _bus(bus),
    _fan(fan),
    _name(FAN_SENSOR_PATH + id),
    _hasTarget(hasTarget),
    _timeout(timeout),
    _timer(events, [this, &fan](){ fan.timerExpired(*this); })
{
    //Load in starting Target and Input values

    try
    {
        // Use getProperty directly to allow a missing sensor object
        // to abort construction.
        _tachInput = util::SDBusPlus::getProperty<decltype(_tachInput)>(
                _bus,
                _name,
                FAN_SENSOR_VALUE_INTF,
                FAN_VALUE_PROPERTY);
    }
    catch (std::exception& e)
    {
        throw InvalidSensorError();
    }

    if (_hasTarget)
    {
        readProperty(FAN_SENSOR_CONTROL_INTF,
                     FAN_TARGET_PROPERTY,
                     _name,
                     _bus,
                     _tachTarget);
    }

    auto match = getMatchString(FAN_SENSOR_VALUE_INTF);

    tachSignal = std::make_unique<sdbusplus::server::match::match>(
                     _bus,
                     match.c_str(),
                     [this](auto& msg){ this->handleTachChange(msg); });

    if (_hasTarget)
    {
        match = getMatchString(FAN_SENSOR_CONTROL_INTF);

        targetSignal = std::make_unique<sdbusplus::server::match::match>(
                           _bus,
                           match.c_str(),
                           [this](auto& msg){ this->handleTargetChange(msg); });
    }

}


std::string TachSensor::getMatchString(const std::string& interface)
{
    return sdbusplus::bus::match::rules::propertiesChanged(
            _name, interface);
}


/**
 * @brief Reads a property from the input message and stores it in value.
 *        T is the value type.
 *
 *        Note: This can only be called once per message.
 *
 * @param[in] msg - the dbus message that contains the data
 * @param[in] interface - the interface the property is on
 * @param[in] propertName - the name of the property
 * @param[out] value - the value to store the property value in
 */
template<typename T>
static void readPropertyFromMessage(sdbusplus::message::message& msg,
                                    const std::string& interface,
                                    const std::string& propertyName,
                                    T& value)
{
    std::string sensor;
    std::map<std::string, sdbusplus::message::variant<T>> data;
    msg.read(sensor, data);

    if (sensor.compare(interface) == 0)
    {
        auto propertyMap = data.find(propertyName);
        if (propertyMap != data.end())
        {
            value = sdbusplus::message::variant_ns::get<T>(
                        propertyMap->second);
        }
    }
}


void TachSensor::handleTargetChange(sdbusplus::message::message& msg)
{
    readPropertyFromMessage(msg,
                            FAN_SENSOR_CONTROL_INTF,
                            FAN_TARGET_PROPERTY,
                            _tachTarget);

    //Check all tach sensors on the fan against the target
    _fan.tachChanged();
}


void TachSensor::handleTachChange(sdbusplus::message::message& msg)
{
   readPropertyFromMessage(msg,
                           FAN_SENSOR_VALUE_INTF,
                           FAN_VALUE_PROPERTY,
                           _tachInput);

   //Check just this sensor against the target
   _fan.tachChanged(*this);
}


std::chrono::microseconds TachSensor::getTimeout()
{
    using namespace std::chrono;

    return duration_cast<microseconds>(seconds(_timeout));
}


}
}
}
