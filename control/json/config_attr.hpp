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

#include "types.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using namespace phosphor::logging;

/**
 * @class ConfigAttr - Configuration attribute
 *
 * Base class for fan control's JSON configuration attributes.
 */
class ConfigAttr
{
  public:
    ConfigAttr() = delete;
    ConfigAttr(const ConfigAttr&) = delete;
    ConfigAttr(ConfigAttr&&) = delete;
    ConfigAttr& operator=(const ConfigAttr&) = delete;
    ConfigAttr& operator=(ConfigAttr&&) = delete;
    virtual ~ConfigAttr() = default;

    explicit ConfigAttr(const json& jsonObj)
    {
        // Set the name of this profile
        setName(jsonObj);
    }

    /**
     * @brief Get the profile's name
     *
     * @return Name of the profile
     */
    inline const std::string& getName()
    {
        return _name;
    }

  protected:
    /**
     * @brief Determines the data type of a JSON configured parameter that is
     * used as a variant within the fan control application and returns the
     * value as that variant.
     * @details Retrieves a JSON object by the first derived data type that
     * is not null. Expected data types should appear in a logical order of
     * conversion. i.e.) uint and int could both be uint
     *
     * @param[in] object - A single JSON object
     *
     * @return A `PropertyVariantType` variant containing the JSON object's
     * value
     */
    static const PropertyVariantType getJsonValue(const json& object)
    {
        if (auto boolPtr = object.get_ptr<const bool*>())
        {
            return *boolPtr;
        }
        if (auto intPtr = object.get_ptr<const int64_t*>())
        {
            return *intPtr;
        }
        if (auto doublePtr = object.get_ptr<const double*>())
        {
            return *doublePtr;
        }
        if (auto stringPtr = object.get_ptr<const std::string*>())
        {
            return *stringPtr;
        }

        log<level::ERR>(
            "Unsupported data type for JSON object's value",
            entry("JSON_ENTRY=%s", object.dump().c_str()),
            entry("SUPPORTED_TYPES=%s", "{bool, int, double, string}"));
        throw std::runtime_error(
            "Unsupported data type for JSON object's value");
    }

  private:
    /* Name of the profile */
    std::string _name;

    /**
     * @brief Sets the profile name from the given JSON
     *
     * @param[in] jsonObj - JSON to get profile name from
     */
    inline void setName(const json& jsonObj)
    {
        if (!jsonObj.contains("name"))
        {
            // Log error on missing profile name
            log<level::ERR>("Missing required profile name",
                            entry("JSON=%s", jsonObj.dump().c_str()));
            throw std::runtime_error("Missing required profile name");
        }
        _name = jsonObj["name"].get<std::string>();
    }
};

} // namespace phosphor::fan::control::json
