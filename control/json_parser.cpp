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

namespace phosphor::fan::control
{

const std::vector<ZoneGroup> getZoneGroups(sdbusplus::bus::bus& bus)
{
    std::vector<ZoneGroup> zoneGrps;

    // Profiles are optional
    auto profiles = getConfig<json::Profile>(bus, true);
    // Fans to be controlled
    auto fans = getConfig<json::Fan>(bus);
    // Zones within the system
    auto zones = getConfig<json::Zone>(bus);
    // Groups to include in events
    auto groups = getConfig<json::Group>(bus);
    // Fan control events
    auto events = getConfig<json::Event>(bus, true);

    // TODO Create zone groups after loading all JSON config files

    return zoneGrps;
}

const unsigned int getPowerOnDelay(sdbusplus::bus::bus& bus)
{
    json::Manager mgr{bus};
    return mgr.getPowerOnDelay();
}

} // namespace phosphor::fan::control
