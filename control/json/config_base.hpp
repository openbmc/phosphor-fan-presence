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

#include <vector>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using namespace phosphor::logging;

using PropertyVariantType =
    std::variant<bool, int32_t, int64_t, double, std::string>;

/**
 * Configuration object key to uniquely map to the configuration object
 * Pair constructed of:
 *      std::string = Configuration object's name
 *      std::vector<std::string> = List of profiles the configuration object
 *                                 is included in
 */
using configKey = std::pair<std::string, std::vector<std::string>>;

/**
 * @class ConfigBase - Base configuration object
 *
 * Base class for fan control's JSON configuration objects.
 */
class ConfigBase
{
  public:
    ConfigBase() = delete;
    ConfigBase(ConfigBase&&) = delete;
    ConfigBase& operator=(const ConfigBase&) = delete;
    ConfigBase& operator=(ConfigBase&&) = delete;

    virtual ~ConfigBase() = default;

    explicit ConfigBase(const json& jsonObj)
    {
        // Set the name of this configuration object
        setName(jsonObj);
        if (jsonObj.contains("profiles"))
        {
            for (const auto& profile : jsonObj["profiles"])
            {
                _profiles.emplace_back(profile.get<std::string>());
            }
        }
    }

    /**
     * Copy Constructor
     * Creates a config base from another config base's originally parsed JSON
     * object data
     *
     * @param[in] origObj - Original ConfigBase object to be created from
     */
    ConfigBase(const ConfigBase& origObj)
    {
        _name = origObj._name;
        _profiles = origObj._profiles;
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

    /**
     * @brief Get the configuration object's list of profiles
     *
     * Gets the list of profiles this configuration object belongs to if any
     * are configured, otherwise an empty list of profiles results in the
     * object always being included in the configuration.
     *
     * @return List of profiles the configuration object belongs to
     */
    inline const auto& getProfiles() const
    {
        return _profiles;
    }

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

  protected:
    /* Name of the configuration object */
    std::string _name;

    /**
     * Profiles this configuration object belongs to (OPTIONAL).
     * Otherwise always include this object in the configuration
     * when no profiles are given
     */
    std::vector<std::string> _profiles;

  private:
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
