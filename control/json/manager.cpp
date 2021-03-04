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

#include "fan.hpp"
#include "json_config.hpp"
#include "profile.hpp"
#include "zone.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

#include <algorithm>
#include <filesystem>
#include <vector>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

std::vector<std::string> Manager::_activeProfiles;

Manager::Manager(sdbusplus::bus::bus& bus, const sdeventplus::Event& event) :
    _bus(bus), _event(event)
{
    // Manager JSON config file is optional
    auto confFile =
        fan::JsonConfig::getConfFile(bus, confAppName, confFileName, true);
    if (!confFile.empty())
    {
        _jsonObj = fan::JsonConfig::load(confFile);
    }

    // Parse and set the available profiles and which are active
    setProfiles();

    // Load the zone configurations
    _zones = getConfig<Zone>(bus);

    // Load the fan configurations and move each fan into its zone
    auto fans = getConfig<Fan>(bus);
    for (auto& fan : fans)
    {
        auto itZone =
            std::find_if(_zones.begin(), _zones.end(),
                         [&fanZone = fan.second->getZone()](const auto& zone) {
                             return fanZone == zone.second->getName();
                         });
        if (itZone != _zones.end())
        {
            itZone->second->addFan(std::move(fan.second));
        }
    }
}

const std::vector<std::string>& Manager::getActiveProfiles()
{
    return _activeProfiles;
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

void Manager::setProfiles()
{
    // Profiles JSON config file is optional
    auto confFile = fan::JsonConfig::getConfFile(_bus, confAppName,
                                                 Profile::confFileName, true);
    if (!confFile.empty())
    {
        for (const auto& entry : fan::JsonConfig::load(confFile))
        {
            auto obj = std::make_unique<Profile>(entry);
            _profiles.emplace(
                std::make_pair(obj->getName(), obj->getProfiles()),
                std::move(obj));
        }
    }
    // Ensure all configurations use the same set of active profiles
    // (In case a profile's active state changes during configuration)
    for (const auto& profile : _profiles)
    {
        if (profile.second->isActive())
        {
            _activeProfiles.emplace_back(profile.first.first);
        }
    }
}

} // namespace phosphor::fan::control::json
