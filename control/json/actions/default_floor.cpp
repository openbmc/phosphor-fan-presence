/**
 * Copyright © 2021 IBM Corporation
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
#include "default_floor.hpp"

#include "../manager.hpp"
#include "../zone.hpp"
#include "group.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

DefaultFloor::DefaultFloor(const json& jsonObj,
                           const std::vector<Group>& groups) :
    ActionBase(jsonObj, groups)
{
    // There are no JSON configuration parameters for this action
}

void DefaultFloor::run(Zone& zone)
{
    for (const auto& group : _groups)
    {
        const auto& members = group.getMembers();
        auto isMissingOwner =
            std::any_of(members.begin(), members.end(),
                        [&intf = group.getInterface()](const auto& member) {
                            return !Manager::hasOwner(member, intf);
                        });
        if (isMissingOwner)
        {
            zone.setFloor(zone.getDefaultFloor());
        }
        // Update fan control floor change allowed
        zone.setFloorChangeAllow(group.getName(), !isMissingOwner);
    }
}

} // namespace phosphor::fan::control::json
