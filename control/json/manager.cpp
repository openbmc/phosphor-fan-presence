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

#include "manager.hpp"

#include "action.hpp"
#include "fan.hpp"
#include "group.hpp"
#include "json_config.hpp"
#include "profile.hpp"
#include "zone.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

std::vector<std::string> Manager::_activeProfiles;
std::map<std::string,
         std::map<std::pair<std::string, bool>, std::vector<std::string>>>
    Manager::_servTree;
std::map<std::string,
         std::map<std::string, std::map<std::string, PropertyVariantType>>>
    Manager::_objects;

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

    // Assign the manager to each zone
    std::for_each(_zones.begin(), _zones.end(),
                  [this](auto& zone) { zone.second->setManager(this); });

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

    bus.request_name(CONTROL_BUSNAME);
}

const std::vector<std::string>& Manager::getActiveProfiles()
{
    return _activeProfiles;
}

bool Manager::hasOwner(const std::string& path, const std::string& intf)
{
    auto itServ = _servTree.find(path);
    if (itServ == _servTree.end())
    {
        // Path not found in cache, therefore owner missing
        return false;
    }
    for (const auto& serv : itServ->second)
    {
        auto itIntf = std::find_if(
            serv.second.begin(), serv.second.end(),
            [&intf](const auto& interface) { return intf == interface; });
        if (itIntf != std::end(serv.second))
        {
            // Service found, return owner state
            return serv.first.second;
        }
    }
    // Interface not found in cache, therefore owner missing
    return false;
}

void Manager::addTimer(const TimerType type,
                       const std::chrono::microseconds interval,
                       std::unique_ptr<TimerPkg> pkg)
{
    auto dataPtr =
        std::make_unique<TimerData>(std::make_pair(type, std::move(*pkg)));
    Timer timer(_event,
                std::bind(&Manager::timerExpired, this, std::ref(*dataPtr)));
    if (type == TimerType::repeating)
    {
        timer.restart(interval);
    }
    else if (type == TimerType::oneshot)
    {
        timer.restartOnce(interval);
    }
    else
    {
        throw std::invalid_argument("Invalid Timer Type");
    }
    _timers.emplace_back(std::make_pair(std::move(dataPtr), std::move(timer)));
}

void Manager::timerExpired(TimerData& data)
{
    auto& actions =
        std::get<std::vector<std::unique_ptr<ActionBase>>&>(data.second);
    // Perform the actions in the timer data
    std::for_each(actions.begin(), actions.end(),
                  [zone = std::ref(std::get<Zone&>(data.second)),
                   group = std::cref(std::get<const Group&>(data.second))](
                      auto& action) { action->run(zone, group); });

    // Remove oneshot timers after they expired
    if (data.first == TimerType::oneshot)
    {
        auto itTimer = std::find_if(
            _timers.begin(), _timers.end(), [&data](const auto& timer) {
                return (data.first == timer.first->first &&
                        (std::get<std::string>(data.second) ==
                         std::get<std::string>(timer.first->second)));
            });
        if (itTimer != std::end(_timers))
        {
            _timers.erase(itTimer);
        }
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
