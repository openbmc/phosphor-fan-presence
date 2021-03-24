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
#include <cstdlib>
#include <string>
#include <tuple>
#include <vector>

namespace phosphor::fan::control
{

bool checkEntry(const std::vector<std::string>& activeProfiles,
                const std::vector<std::string>& entryProfiles)
{
    // Include entry if its list of profiles to be included in is empty
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

const unsigned int getPowerOnDelay(sdbusplus::bus::bus& bus,
                                   const sdeventplus::Event& event)
{
    json::Manager mgr{bus, event};
    return mgr.getPowerOnDelay();
}

} // namespace phosphor::fan::control
