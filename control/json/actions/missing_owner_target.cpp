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
#include "missing_owner_target.hpp"

#include "../manager.hpp"
#include "../zone.hpp"
#include "group.hpp"

#include <fmt/format.h>

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

#include <algorithm>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using namespace phosphor::logging;

MissingOwnerTarget::MissingOwnerTarget(const json& jsonObj) :
    ActionBase(jsonObj)
{
    setTarget(jsonObj);
}

void MissingOwnerTarget::run(Zone& zone, const Group& group)
{
    const auto& members = group.getMembers();
    auto isMissingOwner =
        std::any_of(members.begin(), members.end(),
                    [&intf = group.getInterface()](const auto& member) {
                        return !Manager::hasOwner(member, intf);
                    });
    if (isMissingOwner)
    {
        zone.setTarget(_target);
    }
    // Update group's fan control active allowed based on action results
    zone.setActiveAllow(group.getName(), !isMissingOwner);
}

void MissingOwnerTarget::setTarget(const json& jsonObj)
{
    if (!jsonObj.contains("speed"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required speed value"};
    }
    _target = jsonObj["speed"].get<uint64_t>();
}

} // namespace phosphor::fan::control::json
