/**
 * Copyright © 2020 IBM Corporation
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
#include "zone.hpp"

#include "../zone.hpp"
#include "fan.hpp"
#include "functor.hpp"
#include "handlers.hpp"
#include "types.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

#include <iterator>
#include <map>
#include <numeric>
#include <utility>
#include <vector>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using namespace phosphor::logging;

const std::map<std::string, std::map<std::string, propHandler>>
    Zone::_intfPropHandlers = {{thermModeIntf,
                                {{supportedProp, zone::property::supported},
                                 {currentProp, zone::property::current}}}};

Zone::Zone(sdbusplus::bus::bus& bus, const json& jsonObj) :
    ConfigBase(jsonObj), _incDelay(0)
{
    if (jsonObj.contains("profiles"))
    {
        for (const auto& profile : jsonObj["profiles"])
        {
            _profiles.emplace_back(profile.get<std::string>());
        }
    }
    // Speed increase delay is optional, defaults to 0
    if (jsonObj.contains("increase_delay"))
    {
        _incDelay = jsonObj["increase_delay"].get<uint64_t>();
    }
    setFullSpeed(jsonObj);
    setDefaultFloor(jsonObj);
    setDecInterval(jsonObj);
    // Setting properties on interfaces to be served are optional
    if (jsonObj.contains("interfaces"))
    {
        setInterfaces(jsonObj);
    }
}

void Zone::addFan(std::unique_ptr<Fan> fan)
{
    _fans.emplace_back(std::move(fan));
}

void Zone::setFullSpeed(const json& jsonObj)
{
    if (!jsonObj.contains("full_speed"))
    {
        log<level::ERR>("Missing required zone's full speed",
                        entry("JSON=%s", jsonObj.dump().c_str()));
        throw std::runtime_error("Missing required zone's full speed");
    }
    _fullSpeed = jsonObj["full_speed"].get<uint64_t>();
}

void Zone::setDefaultFloor(const json& jsonObj)
{
    if (!jsonObj.contains("default_floor"))
    {
        log<level::ERR>("Missing required zone's default floor speed",
                        entry("JSON=%s", jsonObj.dump().c_str()));
        throw std::runtime_error("Missing required zone's default floor speed");
    }
    _defaultFloor = jsonObj["default_floor"].get<uint64_t>();
}

void Zone::setDecInterval(const json& jsonObj)
{
    if (!jsonObj.contains("decrease_interval"))
    {
        log<level::ERR>("Missing required zone's decrease interval",
                        entry("JSON=%s", jsonObj.dump().c_str()));
        throw std::runtime_error("Missing required zone's decrease interval");
    }
    _decInterval = jsonObj["decrease_interval"].get<uint64_t>();
}

void Zone::setInterfaces(const json& jsonObj)
{
    for (const auto& interface : jsonObj["interfaces"])
    {
        if (!interface.contains("name") || !interface.contains("properties"))
        {
            log<level::ERR>("Missing required zone interface attributes",
                            entry("JSON=%s", interface.dump().c_str()));
            throw std::runtime_error(
                "Missing required zone interface attributes");
        }
        auto propFuncs =
            _intfPropHandlers.find(interface["name"].get<std::string>());
        if (propFuncs == _intfPropHandlers.end())
        {
            // Construct list of available configurable interfaces
            auto intfs = std::accumulate(
                std::next(_intfPropHandlers.begin()), _intfPropHandlers.end(),
                _intfPropHandlers.begin()->first, [](auto list, auto intf) {
                    return std::move(list) + ", " + intf.first;
                });
            log<level::ERR>("Configured interface not available",
                            entry("JSON=%s", interface.dump().c_str()),
                            entry("AVAILABLE_INTFS=%s", intfs.c_str()));
            throw std::runtime_error("Configured interface not available");
        }

        for (const auto& property : interface["properties"])
        {
            if (!property.contains("name"))
            {
                log<level::ERR>(
                    "Missing required interface property attributes",
                    entry("JSON=%s", property.dump().c_str()));
                throw std::runtime_error(
                    "Missing required interface property attributes");
            }
            // Attribute "persist" is optional, defaults to `false`
            auto persist = false;
            if (property.contains("persist"))
            {
                persist = property["persist"].get<bool>();
            }
            // Property name from JSON must exactly match supported
            // index names to functions in property namespace
            auto propFunc =
                propFuncs->second.find(property["name"].get<std::string>());
            if (propFunc == propFuncs->second.end())
            {
                // Construct list of available configurable properties
                auto props = std::accumulate(
                    std::next(propFuncs->second.begin()),
                    propFuncs->second.end(), propFuncs->second.begin()->first,
                    [](auto list, auto prop) {
                        return std::move(list) + ", " + prop.first;
                    });
                log<level::ERR>("Configured property not available",
                                entry("JSON=%s", property.dump().c_str()),
                                entry("AVAILABLE_PROPS=%s", props.c_str()));
                throw std::runtime_error(
                    "Configured property function not available");
            }
            auto zHandler = propFunc->second(property, persist);
            // Only add non-null zone handler functions
            if (zHandler)
            {
                _zoneHandlers.emplace_back(zHandler);
            }
        }
    }
}

/**
 * Properties of interfaces supported by the zone configuration that return
 * a ZoneHandler function that sets the zone's property value(s).
 */
namespace zone::property
{
// Get a zone handler function for the configured values of the "Supported"
// property
ZoneHandler supported(const json& jsonObj, bool persist)
{
    std::vector<std::string> values;
    if (!jsonObj.contains("values"))
    {
        log<level::ERR>(
            "No 'values' found for \"Supported\" property, using an empty list",
            entry("JSON=%s", jsonObj.dump().c_str()));
    }
    else
    {
        for (const auto& value : jsonObj["values"])
        {
            if (!value.contains("value"))
            {
                log<level::ERR>("No 'value' found for \"Supported\" property "
                                "entry, skipping",
                                entry("JSON=%s", value.dump().c_str()));
            }
            else
            {
                values.emplace_back(value["value"].get<std::string>());
            }
        }
    }

    return make_zoneHandler(handler::setZoneProperty<std::vector<std::string>>(
        Zone::thermModeIntf, Zone::supportedProp, &control::Zone::supported,
        std::move(values), persist));
}

// Get a zone handler function for a configured value of the "Current"
// property
ZoneHandler current(const json& jsonObj, bool persist)
{
    // Use default value for "Current" property if no "value" entry given
    if (!jsonObj.contains("value"))
    {
        log<level::ERR>("No 'value' found for \"Current\" property, "
                        "using default",
                        entry("JSON=%s", jsonObj.dump().c_str()));
        return {};
    }

    return make_zoneHandler(handler::setZoneProperty<std::string>(
        Zone::thermModeIntf, Zone::currentProp, &control::Zone::current,
        jsonObj["value"].get<std::string>(), persist));
}
} // namespace zone::property

} // namespace phosphor::fan::control::json
