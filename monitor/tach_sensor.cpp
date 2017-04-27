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
#include "tach_sensor.hpp"
#include "../utility.hpp"

namespace phosphor
{
namespace fan
{
namespace monitor
{

constexpr auto PROPERTY_INTF = "org.freedesktop.DBus.Properties";
constexpr auto FAN_SENSOR_PATH = "/xyz/openbmc_project/sensors/fan_tach/";
constexpr auto FAN_SENSOR_CONTROL_INTF = "xyz.openbmc_project.Control.FanSpeed";
constexpr auto FAN_SENSOR_VALUE_INTF = "xyz.openbmc_project.Sensor.Value";
constexpr auto FAN_TARGET_PROPERTY = "Target";
constexpr auto FAN_VALUE_PROPERTY = "Value";


TachSensor::TachSensor(sdbusplus::bus::bus& bus,
                       Fan& fan,
                       const std::string& id,
                       bool hasTarget,
                       size_t startupTimeout,
                       size_t timeout,
                       sd_event* events) :
    _bus(bus),
    _fan(fan),
    _name(FAN_SENSOR_PATH + id),
    _hasTarget(hasTarget),
    _startupTimeout(startupTimeout),
    _timeout(timeout),
    _timer(events, std::bind(&Fan::timerExpired, &fan, this))
{
    //Load in a known starting Target value
    if (_hasTarget)
    {
        readTargetSpeed();
    }

    auto match = getMatchString(FAN_SENSOR_VALUE_INTF);

    tachSignal = std::make_unique<sdbusplus::server::match::match>(
                     _bus,
                     match.c_str(),
                     handleTachChangeSignal,
                     this);

    if (_hasTarget)
    {
        match = getMatchString(FAN_SENSOR_CONTROL_INTF);

        targetSignal = std::make_unique<sdbusplus::server::match::match>(
                           _bus,
                           match.c_str(),
                           handleTargetChangeSignal,
                           this);
    }

}


//Can cache this value after openbmc/openbmc#1496 is resolved
std::string TachSensor::getService()
{
    return phosphor::fan::util::getService(_name,
                                           FAN_SENSOR_CONTROL_INTF,
                                           _bus);
}


std::string TachSensor::getMatchString(const std::string& interface)
{
    return std::string("type='signal',"
                       "interface='org.freedesktop.DBus.Properties',"
                       "member='PropertiesChanged',"
                       "arg0namespace='" + interface + "',"
                       "path='" + _name + "'");
}


void TachSensor::readTargetSpeed()
{
    sdbusplus::message::variant<uint64_t> property;
    std::string propertyName{FAN_TARGET_PROPERTY};

    try
    {
        auto service = getService();

        auto method = _bus.new_method_call(service.c_str(),
                                           _name.c_str(),
                                           PROPERTY_INTF,
                                           "Get");
        method.append(FAN_SENSOR_CONTROL_INTF, propertyName);

        auto reply = _bus.call(method);
        if (reply.is_method_error())
        {
            throw std::runtime_error(
                "Error in Target property get call for sensor " +
                _name);
        }

        reply.read(property);

        _tachTarget = sdbusplus::message::
                      variant_ns::get<uint64_t>(property);
    }
    catch (std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
    }
}


int TachSensor::handleTachChangeSignal(sd_bus_message* msg,
                                       void* usrData,
                                       sd_bus_error* err)
{
    auto m = sdbusplus::message::message(msg);
    static_cast<TachSensor*>(usrData)->handleTachChange(m, err);
    return 0;
}


int TachSensor::handleTargetChangeSignal(sd_bus_message* msg,
                                         void* usrData,
                                         sd_bus_error* err)
{
    auto m = sdbusplus::message::message(msg);
    static_cast<TachSensor*>(usrData)->handleTargetChange(m, err);
    return 0;
}


/**
 * @brief Reads a property from the input message and stores it in value.
 *        T is the value type.
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


void TachSensor::handleTargetChange(sdbusplus::message::message& msg,
                                    sd_bus_error* err)
{
    readPropertyFromMessage(msg,
                            FAN_SENSOR_CONTROL_INTF,
                            FAN_TARGET_PROPERTY,
                            _tachTarget);

    //Check all tach sensors on the fan against the target
    _fan.tachChanged();
}


void TachSensor::handleTachChange(sdbusplus::message::message& msg,
                                  sd_bus_error* err)
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
    size_t timeout;

    //Just use the startup value once
    if (_startupTimeout != 0)
    {
        timeout = _startupTimeout;
        _startupTimeout = 0;
    }
    else
    {
        timeout = _timeout;
    }

    return duration_cast<microseconds>(seconds(timeout));
}


}
}
}
