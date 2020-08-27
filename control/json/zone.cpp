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

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

#include <any>
#include <iterator>
#include <map>
#include <numeric>
#include <tuple>
#include <utility>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using namespace phosphor::logging;

const std::map<std::string, propertyHandler> Zone::_props = {
    {"Supported", property::supported}, {"Current", property::current}};

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
        std::map<std::string, std::tuple<std::any, bool>> props;
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
            auto prop = property["name"].get<std::string>();
            auto propFunc = _props.find(prop);
            if (propFunc != _props.end())
            {
                auto value = propFunc->second(property);
                props.emplace(prop, std::make_tuple(value, persist));
            }
            else
            {
                // Construct list of available properties
                auto props = std::accumulate(
                    std::next(_props.begin()), _props.end(),
                    _props.begin()->first, [](auto list, auto prop) {
                        return std::move(list) + ", " + prop.first;
                    });
                log<level::ERR>("Configured property function not available",
                                entry("JSON=%s", property.dump().c_str()),
                                entry("AVAILABLE_PROPS=%s", props.c_str()));
            }
        }
        _interfaces.emplace(interface["name"].get<std::string>(), props);
    }
}

/**
 * Properties of interfaces supported by the zone configuration
 */
namespace property
{
// Get an any object for the configured value of the "Supported" property
std::any supported(const json& jsonObj)
{
    std::vector<std::string> values;
    for (const auto& value : jsonObj["values"])
    {
        values.emplace_back(value["value"].get<std::string>());
    }

    return std::make_any<std::vector<std::string>>(values);
}

// Get an any object for the configured value of the "Current" property
std::any current(const json& jsonObj)
{
    auto value = jsonObj["value"].get<std::string>();
    return std::make_any<std::string>(value);
}
} // namespace property

} // namespace phosphor::fan::control::json
