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
#pragma once

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using namespace phosphor::logging;

/**
 * @class ConfigBase - Base configuration object
 *
 * Base class for fan control's JSON configuration objects.
 */
class ConfigBase
{
  public:
    ConfigBase() = delete;
    ConfigBase(const ConfigBase&) = delete;
    ConfigBase(ConfigBase&&) = delete;
    ConfigBase& operator=(const ConfigBase&) = delete;
    ConfigBase& operator=(ConfigBase&&) = delete;
    virtual ~ConfigBase() = default;

    explicit ConfigBase(const json& jsonObj)
    {
        // Set the name of this configuration object
        setName(jsonObj);
    }

    /**
     * @brief Get the configuration object's name
     *
     * @return Name of the configuration object
     */
    inline const std::string& getName() const
    {
        return _name;
    }

  private:
    /* Name of the configuration object */
    std::string _name;

    /**
     * @brief Sets the configuration object's name from the given JSON
     *
     * @param[in] jsonObj - JSON to get configuration object's name from
     */
    inline void setName(const json& jsonObj)
    {
        if (!jsonObj.contains("name"))
        {
            // Log error on missing configuration object's name
            log<level::ERR>("Missing required configuration object's name",
                            entry("JSON=%s", jsonObj.dump().c_str()));
            throw std::runtime_error(
                "Missing required configuration object's name");
        }
        _name = jsonObj["name"].get<std::string>();
    }
};

} // namespace phosphor::fan::control::json
