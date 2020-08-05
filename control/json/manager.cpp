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
#include "manager.hpp"

#include "json_config.hpp"
#include "json_parser.hpp"

#include <sdbusplus/bus.hpp>

#include <filesystem>

namespace phosphor::fan::control::json
{

Manager::Manager(sdbusplus::bus::bus& bus)
{
    // Manager JSON config file is optional
    auto confFile =
        fan::JsonConfig::getConfFile(bus, confAppName, confFileName, true);
    if (!confFile.empty())
    {
        _jsonObj = fan::JsonConfig::load(confFile);
    }
}

unsigned int Manager::getPowerOnDelay()
{
    auto powerOnDelay = 0;

    // Parse optional "power_on_delay" from JSON object
    if (!_jsonObj.empty() && _jsonObj.contains("power_on_delay"))
    {
        powerOnDelay = _jsonObj["power_on_delay"].get<unsigned int>();
    }

    return powerOnDelay;
}

} // namespace phosphor::fan::control::json
