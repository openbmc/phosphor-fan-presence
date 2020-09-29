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
#include "json_parser.hpp"

#include "json/event.hpp"
#include "json/fan.hpp"
#include "json/group.hpp"
#include "json/manager.hpp"
#include "json/profile.hpp"
#include "json/zone.hpp"
#include "types.hpp"

#include <sdbusplus/bus.hpp>

#include <algorithm>
#include <string>
#include <tuple>
#include <vector>

namespace phosphor::fan::control
{

const bool addEntry(const std::vector<std::string>& activeProfiles,
                    const std::vector<std::string>& entryProfiles)
{
    // Include entry if it's list of profiles to be included in is empty
    if (entryProfiles.empty())
    {
        // Entry always to be included
        return true;
    }
    else
    {
        for (const auto& profile : activeProfiles)
        {
            auto iter =
                std::find(entryProfiles.begin(), entryProfiles.end(), profile);
            if (iter != entryProfiles.end())
            {
                // Entry configured to be included in active profile
                return true;
            }
        }
    }
    return false;
}

const std::vector<ZoneGroup> getZoneGroups(sdbusplus::bus::bus& bus)
{
    std::vector<ZoneGroup> zoneGrps;

    // Profiles are optional
    auto profiles = getConfig<json::Profile>(bus, true);
    // Fans to be controlled
    auto fans = getConfig<json::Fan>(bus);
    // Zones within the system
    auto zones = getConfig<json::Zone>(bus);
    // Fan control events are optional
    auto events = getConfig<json::Event>(bus, true);
    if (!events.empty())
    {
        // Groups to include in events
        auto groups = getConfig<json::Group>(bus);
    }

    // Ensure all configurations use the same set of active profiles
    // (In case a profile's active state changes during configuration)
    std::vector<std::string> activeProfiles;
    for (const auto& profile : profiles)
    {
        if (profile.second->isActive())
        {
            activeProfiles.emplace_back(profile.first.first);
        }
    }

    // Conditions list empty for JSON based configurations
    // TODO Remove conditions after YAML based configuration removed
    std::vector<Condition> conditions;
    std::vector<ZoneDefinition> zoneDefs;
    for (const auto& zone : zones)
    {
        // Check zone profiles against active profiles
        if (addEntry(activeProfiles, zone.second->getProfiles()))
        {
            auto zoneName = zone.second->getName();
            // Create FanDefinition list for zone
            std::vector<FanDefinition> fanDefs;
            for (const auto& fan : fans)
            {
                // Check fan profiles against active profiles and
                // if the fan is included in the zone
                if (addEntry(activeProfiles, fan.second->getProfiles()) &&
                    fan.second->getZone() == zoneName)
                {
                    fanDefs.emplace_back(std::make_tuple(
                        fan.second->getName(), fan.second->getSensors(),
                        fan.second->getInterface()));
                }
            }

            // Create SetSpeedEvents list for zone
            std::vector<SetSpeedEvent> speedEvents;
            // TODO Populate SetSpeedEvents list with configured events

            // YAML zone name was an integer, JSON is a string
            // Design direction is for it to be a string
            zoneDefs.emplace_back(std::make_tuple(
                std::stoi(zoneName), zone.second->getFullSpeed(),
                zone.second->getDefaultFloor(), zone.second->getIncDelay(),
                zone.second->getDecInterval(), zone.second->getZoneHandlers(),
                fanDefs, speedEvents));
        }
    }
    // TODO Should only result in a single entry but YAML based configurations
    // produce a list of zone groups. Change to single zone group after YAML
    // based configuartions are removed.
    zoneGrps.emplace_back(std::make_tuple(conditions, zoneDefs));

    return zoneGrps;
}

const unsigned int getPowerOnDelay(sdbusplus::bus::bus& bus)
{
    json::Manager mgr{bus};
    return mgr.getPowerOnDelay();
}

} // namespace phosphor::fan::control
