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
#include "request_target_base.hpp"

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

RequestTargetBase::RequestTargetBase(const json& jsonObj,
                                     const std::vector<Group>& groups) :
    ActionBase(jsonObj, groups)
{
    // There are no JSON configuration parameters for this action
}

void RequestTargetBase::run(Zone& zone)
{
    uint64_t base = 0;
    for (const auto& group : _groups)
    {
        for (const auto& member : group.getMembers())
        {
            try
            {
                auto value = Manager::getObjValueVariant(
                    member, group.getInterface(), group.getProperty());
                if (auto intPtr = std::get_if<int64_t>(&value))
                {
                    // Throw out any negative values as those are not valid
                    // to use as a fan target base
                    if (*intPtr < 0)
                    {
                        continue;
                    }
                    base = std::max(base, static_cast<uint64_t>(*intPtr));
                }
                else if (auto dblPtr = std::get_if<double>(&value))
                {
                    // Throw out any negative values as those are not valid
                    // to use as a fan target base
                    if (*dblPtr < 0)
                    {
                        continue;
                    }
                    // Precision of a double not a concern with fan targets
                    base = std::max(base, static_cast<uint64_t>(*dblPtr));
                }
                else
                {
                    // Unsupported group member type for this action
                    log<level::ERR>(
                        fmt::format("Action {}: Unsupported group member type "
                                    "given. [object = {} : {} : {}]",
                                    getName(), member, group.getInterface(),
                                    group.getProperty())
                            .c_str());
                }
            }
            catch (const std::out_of_range& oore)
            {
                // Property value not found, base request target unchanged
            }
        }
    }

    // A request target base of 0 defaults to the current target
    zone.setRequestTargetBase(base);
}

} // namespace phosphor::fan::control::json
