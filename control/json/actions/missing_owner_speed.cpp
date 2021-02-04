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
#include "missing_owner_speed.hpp"

#include "types.hpp"
#include "zone.hpp"

#include <fmt/format.h>

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

#include <algorithm>
#include <tuple>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using namespace phosphor::logging;

MissingOwnerSpeed::MissingOwnerSpeed(const json& jsonObj) : ActionBase(jsonObj)
{
    setSpeed(jsonObj);
}

void MissingOwnerSpeed::run(Zone& zone, const Group& group)
{
    // Set/update the services of the group
    zone.setServices(&group);
    auto services = zone.getGroupServices(&group);
    auto missingOwner =
        std::any_of(services.begin(), services.end(),
                    [](const auto& s) { return !std::get<hasOwnerPos>(s); });
    if (missingOwner)
    {
        zone.setSpeed(_speed);
    }
    // Update group's fan control active allowed based on action results
    zone.setActiveAllow(&group, !missingOwner);
}

void MissingOwnerSpeed::setSpeed(const json& jsonObj)
{
    if (!jsonObj.contains("speed"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required speed value"};
    }
    _speed = jsonObj["speed"].get<uint64_t>();
}

} // namespace phosphor::fan::control::json
