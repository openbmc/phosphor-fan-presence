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
#include <algorithm>
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <unistd.h>
#include "manager.hpp"
#include "utility.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{

using namespace phosphor::logging;

constexpr auto SYSTEMD_SERVICE   = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH  = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";
constexpr auto FAN_CONTROL_READY_TARGET = "obmc-fan-control-ready@0.target";

constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";
constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";


/**
 * Get the current value of the D-Bus property under the specified path
 * and interface.
 *
 * @param[in] bus          - The D-Bus bus object
 * @param[in] path         - The D-Bus path
 * @param[in] interface    - The D-Bus interface
 * @param[in] propertyName - The D-Bus property
 * @param[out] value       - The D-Bus property's value
 */
template <typename T>
void getProperty(sdbusplus::bus::bus& bus,
                 const std::string& path,
                 const std::string& interface,
                 const std::string& propertyName,
                 T& value)
{
    sdbusplus::message::variant<T> property;
    std::string service = phosphor::fan::util::getService(path, interface, bus);

    auto method = bus.new_method_call(service.c_str(),
                                      path.c_str(),
                                      PROPERTY_INTERFACE,
                                      "Get");

    method.append(interface, propertyName);
    auto reply = bus.call(method);

    if (reply.is_method_error())
    {
        log<level::ERR>("Error in call response for retrieving property");
        elog<InternalFailure>();
    }
    reply.read(property);
    value = sdbusplus::message::variant_ns::get<T>(property);
}


/**
 * Check if a condition is true. Conditions are used to determine
 * which fan zone to use.
 *
 * @param[in] bus       - The D-Bus bus object
 * @param[in] condition - The condition to check if true
 * @return result       - True if the condition is true
 */
bool checkCondition(sdbusplus::bus::bus& bus, const auto& c)
{
    auto& type = std::get<conditionTypePos>(c);
    auto& properties = std::get<conditionPropertyListPos>(c);

    for (auto& p : properties)
    {
        bool value = std::get<propertyValuePos>(p);
        bool propertyValue;

        // TODO openbmc/openbmc#1769: Support more types than just getProperty.
        if (type.compare("getProperty") == 0)
        {
            getProperty(bus,
                        std::get<propertyPathPos>(p),
                        std::get<propertyInterfacePos>(p),
                        std::get<propertyNamePos>(p),
                        propertyValue);

            if (value != propertyValue)
            {
                return false;
            }
        }
    }
    return true;
}


//Note: Future code will check 'mode' before starting control algorithm
Manager::Manager(sdbusplus::bus::bus& bus,
                 phosphor::fan::event::EventPtr& events,
                 Mode mode) :
    _bus(bus)
{
    //Create the appropriate Zone objects based on the
    //actual system configuration.

    //Find the 1 ZoneGroup that meets all of its conditions
    for (auto& group : _zoneLayouts)
    {
        auto& conditions = std::get<conditionListPos>(group);

        if (std::all_of(conditions.begin(), conditions.end(),
                        [&bus](const auto& condition)
        {
            return checkCondition(bus, condition);
        }))
        {
            //Create a Zone object for each zone in this group
            auto& zones = std::get<zoneListPos>(group);

            for (auto& z : zones)
            {
                _zones.emplace(std::get<zoneNumPos>(z),
                               std::make_unique<Zone>(mode, _bus, events, z));
            }

            break;
        }
    }

}


void Manager::doInit()
{
    for (auto& z : _zones)
    {
        z.second->setFullSpeed();
    }

    auto delay = _powerOnDelay;
    while (delay > 0)
    {
        delay = sleep(delay);
    }

    startFanControlReadyTarget();
}


void Manager::startFanControlReadyTarget()
{
    auto method = _bus.new_method_call(SYSTEMD_SERVICE,
                                       SYSTEMD_OBJ_PATH,
                                       SYSTEMD_INTERFACE,
                                       "StartUnit");

    method.append(FAN_CONTROL_READY_TARGET);
    method.append("replace");

    auto response = _bus.call(method);
    if (response.is_method_error())
    {
        log<level::ERR>("Failed to start fan control ready target");
        elog<InternalFailure>();
    }
}

} // namespace control
} // namespace fan
} // namespace phosphor
