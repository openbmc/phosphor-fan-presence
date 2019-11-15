/**
 * Copyright © 2019 IBM Corporation
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
#include <string>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

#include "json_config.hpp"

namespace phosphor
{
namespace fan
{
namespace presence
{

using json = nlohmann::json;
namespace fs = std::filesystem;
using namespace phosphor::logging;

policies JsonConfig::_policies;

JsonConfig::JsonConfig(const std::string& jsonFile)
{
    fs::path confFile{jsonFile};
    std::ifstream file;
    json jsonConf;

    if (fs::exists(confFile))
    {
        file.open(confFile);
        try
        {
            jsonConf = json::parse(file);
        }
        catch (std::exception& e)
        {
            log<level::ERR>("Failed to parse JSON config file",
                            entry("JSON_FILE=%s", jsonFile.c_str()),
                            entry("JSON_ERROR=%s", e.what()));
            throw std::runtime_error("Failed to parse JSON config file");
        }
    }
    else
    {
        log<level::ERR>("Unable to open JSON config file",
                        entry("JSON_FILE=%s", jsonFile.c_str()));
        throw std::runtime_error("Unable to open JSON config file");
    }
}

const policies& JsonConfig::get()
{
    return _policies;
}

} // namespace presence
} // namespace fan
} // namespace phosphor
