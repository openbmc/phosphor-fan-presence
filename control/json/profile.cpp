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
#include "profile.hpp"

#include "sdbusplus.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

#include <algorithm>
#include <iterator>
#include <numeric>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using namespace phosphor::logging;

// String key must be in all lowercase for method lookup
const std::map<std::string, methodHandler> Profile::_methods = {
    {"all_of", Profile::allOf}};

Profile::Profile(const json& jsonObj) : ConfigBase(jsonObj), _active(false)
{
    setActive(jsonObj);
}

void Profile::setActive(const json& jsonObj)
{
    if (!jsonObj.contains("method") || !jsonObj["method"].contains("name"))
    {
        // Log error on missing profile method
        log<level::ERR>("Missing required profile method",
                        entry("JSON=%s", jsonObj.dump().c_str()));
        throw std::runtime_error("Missing required profile method");
    }
    // The method to use in determining if the profile is active
    auto method = jsonObj["method"]["name"].get<std::string>();
    std::transform(method.begin(), method.end(), method.begin(), tolower);
    auto handler = _methods.find(method);
    if (handler != _methods.end())
    {
        // Call method for determining profile's active state
        _active = handler->second(jsonObj["method"]);
    }
    else
    {
        // Construct list of available methods
        auto methods = std::accumulate(
            std::next(_methods.begin()), _methods.end(),
            _methods.begin()->first, [](auto list, auto method) {
                return std::move(list) + ", " + method.first;
            });
        log<level::ERR>("Configured method not available",
                        entry("JSON=%s", jsonObj["method"].dump().c_str()),
                        entry("METHODS_AVAILABLE=%s", methods.c_str()));
    }
}

bool Profile::allOf(const json& method)
{
    if (!method.contains("properties"))
    {
        log<level::ERR>("Missing required all_of method properties list",
                        entry("JSON=%s", method.dump().c_str()));
        throw std::runtime_error(
            "Missing required all_of method properties list");
    }

    return std::all_of(
        method["properties"].begin(), method["properties"].end(),
        [](const json& obj) {
            if (!obj.contains("path") || !obj.contains("interface") ||
                !obj.contains("property") || !obj.contains("value"))
            {
                log<level::ERR>(
                    "Missing required all_of method property parameters",
                    entry("JSON=%s", obj.dump().c_str()));
                throw std::runtime_error(
                    "Missing required all_of method parameters");
            }
            auto variant =
                util::SDBusPlus::getPropertyVariant<PropertyVariantType>(
                    obj["path"].get<std::string>(),
                    obj["interface"].get<std::string>(),
                    obj["property"].get<std::string>());

            return getJsonValue(obj["value"]) == variant;
        });
}

} // namespace phosphor::fan::control::json
