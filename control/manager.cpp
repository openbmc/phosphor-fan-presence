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
#include "manager.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{

Manager::Manager(sdbusplus::bus::bus& bus) :
    _bus(bus)
{
    //Create the appropriate Zone objects based on the
    //actual system configuration.

    //Find the 1 ZoneGroup that meets all of its conditions
    for (auto& group : _zoneLayouts)
    {
        if (meetsGroupConditions(std::get<conditionListPos>(group)))
        {
            //Create a Zone object for each zone in this group
            auto& zones = std::get<zoneListPos>(group);

            for (auto& z : zones)
            {
                _zones.emplace(std::get<zoneNumPos>(z),
                               std::make_unique<Zone>(_bus, z));
            }

            break;
        }
    }

    //Set to full since we don't know state of system yet.
    //This may be done differently in the future when OCC code ready.
    for (auto& z: _zones)
    {
        z.second->setFullSpeed();
    }
}


bool Manager::meetsGroupConditions(
    const std::vector<Condition> & conditions)
{
    //TODO: check the conditions passed in and return true
    //if they all return true
    return true;
}


}
}
}
