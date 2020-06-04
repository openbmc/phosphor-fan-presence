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

#include "json_config.hpp"
#include "nonzero_speed_trust.hpp"
#include "types.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

#include <algorithm>
#include <map>
#include <memory>
#include <vector>

namespace phosphor::fan::monitor
{

using json = nlohmann::json;
using namespace phosphor::logging;

namespace tClass
{

// Get a constructed trust group class for a non-zero speed group
CreateGroupFunction
    getNonZeroSpeed(const std::vector<trust::GroupDefinition>& group)
{
    return [&group]() {
        return std::make_unique<trust::NonzeroSpeed>(std::move(group));
    };
}

} // namespace tClass

const std::map<std::string, trustHandler> trusts = {
    {"nonzerospeed", tClass::getNonZeroSpeed}};

const std::vector<CreateGroupFunction> getTrustGrps(const json& obj)
{
    std::vector<CreateGroupFunction> grpFuncs;

    if (obj.contains("sensor_trust_groups"))
    {
        for (auto& stg : obj["sensor_trust_groups"])
        {
            if (!stg.contains("class") || !stg.contains("group"))
            {
                // Log error on missing required parameters
                log<level::ERR>(
                    "Missing required fan monitor trust group parameters",
                    entry("REQUIRED_PARAMETERS=%s", "{class, group}"));
                throw std::runtime_error(
                    "Missing required fan trust group parameters");
            }
            auto tgClass = stg["class"].get<std::string>();
            std::vector<trust::GroupDefinition> group;
            for (auto& member : stg["group"].items())
            {
                // Construct list of group members
                if (!member.value().contains("name"))
                {
                    // Log error on missing required parameter
                    log<level::ERR>(
                        "Missing required fan monitor trust group member name",
                        entry("CLASS=%s", tgClass.c_str()));
                    throw std::runtime_error(
                        "Missing required fan monitor trust group member name");
                }
                auto in_trust = true;
                if (member.value().contains("in_trust"))
                {
                    in_trust = member.value()["in_trust"].get<bool>();
                }
                group.emplace_back(trust::GroupDefinition{
                    member.value()["name"].get<std::string>(), in_trust});
            }
            // The class for fan sensor trust groups
            // (Must have a supported function within the tClass namespace)
            std::transform(tgClass.begin(), tgClass.end(), tgClass.begin(),
                           tolower);
            auto handler = trusts.find(tgClass);
            if (handler != trusts.end())
            {
                // Call function for trust group class
                grpFuncs.emplace_back(handler->second(group));
            }
            else
            {
                // Log error on unsupported trust group class
                log<level::ERR>("Invalid fan monitor trust group class",
                                entry("CLASS=%s", tgClass.c_str()));
                throw std::runtime_error(
                    "Invalid fan monitor trust group class");
            }
        }
    }

    return grpFuncs;
}

} // namespace phosphor::fan::monitor
