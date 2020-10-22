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
#include "group.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using namespace phosphor::logging;

Group::Group(sdbusplus::bus::bus& bus, const json& jsonObj) :
    ConfigBase(jsonObj), _service("")
{
    if (jsonObj.contains("profiles"))
    {
        for (const auto& profile : jsonObj["profiles"])
        {
            _profiles.emplace_back(profile.get<std::string>());
        }
    }
    setMembers(jsonObj);
    // Setting the group's service name is optional
    if (jsonObj.contains("service"))
    {
        setService(jsonObj);
    }
}

void Group::setMembers(const json& jsonObj)
{
    if (!jsonObj.contains("members"))
    {
        log<level::ERR>("Missing required group's members",
                        entry("JSON=%s", jsonObj.dump().c_str()));
        throw std::runtime_error("Missing required group's members");
    }
    for (const auto& member : jsonObj["members"])
    {
        _members.emplace_back(member.get<std::string>());
    }
}

void Group::setService(const json& jsonObj)
{
    _service = jsonObj["service"].get<std::string>();
}

} // namespace phosphor::fan::control::json
