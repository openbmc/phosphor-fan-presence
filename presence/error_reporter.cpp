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
#include "error_reporter.hpp"

#include <phosphor-logging/log.hpp>

namespace phosphor::fan::presence
{

using json = nlohmann::json;
using namespace phosphor::logging;

ErrorReporter::ErrorReporter(
    sdbusplus::bus::bus& bus, const json& jsonConf,
    const std::vector<
        std::tuple<Fan, std::vector<std::unique_ptr<PresenceSensor>>>>& fans) :
    _bus(bus)
{
    loadConfig(jsonConf);
}

void ErrorReporter::loadConfig(const json& jsonConf)
{
    if (!jsonConf.contains("fan_missing_error_time"))
    {
        log<level::ERR>("Missing 'fan_missing_error_time' entry in JSON "
                        "'reporting' section");

        throw std::runtime_error("Missing fan missing time entry in JSON");
    }

    _fanMissingErrorTime = std::chrono::seconds{
        jsonConf.at("fan_missing_error_time").get<std::size_t>()};
}

} // namespace phosphor::fan::presence
