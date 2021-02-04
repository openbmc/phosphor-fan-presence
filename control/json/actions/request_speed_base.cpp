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
#include "request_speed_base.hpp"

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

RequestSpeedBase::RequestSpeedBase(const json& jsonObj) : ActionBase(jsonObj)
{
    // There are no JSON configuration parameters for this action
}

void RequestSpeedBase::run(Zone& zone, const Group& group)
{
    uint64_t base = 0;
    for (const auto& member : group)
    {
        try
        {
            auto value = zone.getPropValueVariant(std::get<pathPos>(member),
                                                  std::get<intfPos>(member),
                                                  std::get<propPos>(member));
            if (auto intPtr = std::get_if<int64_t>(&value))
            {
                // Throw out any negative values as those are not valid
                // to use as a fan speed base
                if (*intPtr < 0)
                {
                    continue;
                }
                base = std::max(base, static_cast<uint64_t>(*intPtr));
            }
            else if (auto dblPtr = std::get_if<double>(&value))
            {
                // Throw out any negative values as those are not valid
                // to use as a fan speed base
                if (*dblPtr < 0)
                {
                    continue;
                }
                // Precision of a double not a concern with fan speeds
                base = std::max(base, static_cast<uint64_t>(*dblPtr));
            }
            else
            {
                // Unsupported group member type for this action
                log<level::ERR>(
                    fmt::format("Action {}: Unsupported group member type "
                                "given. [object = {} : {} : {}]",
                                getName(), std::get<pathPos>(member),
                                std::get<intfPos>(member),
                                std::get<propPos>(member))
                        .c_str());
            }
        }
        catch (const std::out_of_range& oore)
        {
            // Property value not found, base request speed unchanged
        }
    }

    // A request speed base of 0 defaults to the current target speed
    zone.setRequestSpeedBase(base);
}

} // namespace phosphor::fan::control::json
