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
#include "config.h"

#include "system.hpp"

#include "fan.hpp"
#include "fan_defs.hpp"
#include "tach_sensor.hpp"
#include "trust_manager.hpp"
#include "types.hpp"
#ifdef MONITOR_USE_JSON
#include "json_parser.hpp"
#endif

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

namespace phosphor::fan::monitor
{

using json = nlohmann::json;

System::System(Mode mode, sdbusplus::bus::bus& bus,
               const sdeventplus::Event& event) :
    _mode(mode),
    _bus(bus), _event(event)
{
    json jsonObj = json::object();
#ifdef MONITOR_USE_JSON
    jsonObj = getJsonObj(bus);
#endif
    // Retrieve and set trust groups within the trust manager
    _trust = std::make_unique<trust::Manager>(getTrustGroups(jsonObj));

    // Retrieve fan definitions and create fan objects to be monitored
    for (const auto& fanDef : getFanDefinitions(jsonObj))
    {
        // Check if a condition exists on the fan
        auto condition = std::get<conditionField>(fanDef);
        if (condition)
        {
            // Condition exists, skip adding fan if it fails
            if (!(*condition)(bus))
            {
                continue;
            }
        }
        _fans.emplace_back(
            std::make_unique<Fan>(mode, bus, event, _trust, fanDef));
    }
}

const std::vector<CreateGroupFunction>
    System::getTrustGroups(const json& jsonObj)
{
#ifdef MONITOR_USE_JSON
    return getTrustGrps(jsonObj);
#else
    return trustGroups;
#endif
}

const std::vector<FanDefinition> System::getFanDefinitions(const json& jsonObj)
{
#ifdef MONITOR_USE_JSON
    return getFanDefs(jsonObj);
#else
    return fanDefinitions;
#endif
}

} // namespace phosphor::fan::monitor
