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

#include "config_base.hpp"

#include <nlohmann/json.hpp>

#include <set>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

/**
 * @class Group - Represents a group of dbus objects for configured events
 *
 * A group contains a list of dbus objects that are logically grouped together
 * to be used within one-or-more configured fan control events. An event object
 * is configured to apply a set of actions against a list of groups that could
 * result in a fan control target change. A group may also be configured against
 * a list of profiles(OPTIONAL) and or denote a specific service(OPTIONAL) that
 * serves the list of dbus objects in the group.
 *
 * (When no profile for a group is given, the group defaults to always be used
 * within the events its included in)
 *
 */
class Group : public ConfigBase
{
  public:
    /* JSON file name for groups */
    static constexpr auto confFileName = "groups.json";

    Group() = delete;
    Group(Group&&) = delete;
    Group& operator=(const Group&) = delete;
    Group& operator=(Group&&) = delete;
    ~Group() = default;

    /**
     * Constructor
     * Parses and populates a configuration group from JSON object data
     *
     * @param[in] jsonObj - JSON object
     */
    explicit Group(const json& jsonObj);

    /**
     * Copy Constructor
     * Creates a group from another group's originally parsed JSON object data
     *
     * @param[in] origObj - Original Group object to be created from
     */
    Group(const Group& origObj);

    /**
     * @brief Get the members
     *
     * @return List of dbus paths representing the members of the group
     */
    inline const auto& getMembers() const
    {
        return _members;
    }

    /**
     * @brief Get the service
     *
     * @return Service name serving the members of the group
     */
    inline const auto& getService() const
    {
        return _service;
    }

    /**
     * @brief Set the dbus interface name for the group
     */
    inline void setInterface(const std::string& intf)
    {
        _interface = intf;
    }

    /**
     * @brief Get the group's dbus interface name
     */
    inline const auto& getInterface() const
    {
        return _interface;
    }

    /**
     * @brief Set the dbus property name for the group
     */
    inline void setProperty(const std::string& prop)
    {
        _property = prop;
    }

    /**
     * @brief Get the group's dbus property name
     */
    inline const auto& getProperty() const
    {
        return _property;
    }

    /**
     * @brief Set the dbus property's data type for the group
     */
    inline void setType(const std::optional<std::string>& type)
    {
        _type = type;
    }

    /**
     * @brief Get the group's dbus property's data type
     */
    inline const auto& getType() const
    {
        return _type;
    }

    /**
     * @brief Set the dbus property's expected value for the group
     */
    inline void setValue(const std::optional<PropertyVariantType>& value)
    {
        _value = value;
    }

    /**
     * @brief Get the group's dbus property's expected value
     */
    inline const auto& getValue() const
    {
        return _value;
    }

    /**
     * @brief Get the set of all configured group members
     */
    static const std::set<std::string>& getAllMembers()
    {
        return _allMembers;
    }

  private:
    /* Members of the group */
    std::vector<std::string> _members;

    /* Service name serving all the members */
    std::string _service;

    /* Dbus interface name for all the members */
    std::string _interface;

    /* Dbus property name for all the members */
    std::string _property;

    /* Optional property's data type for all members */
    std::optional<std::string> _type;

    /* Optional property value for all the members */
    std::optional<PropertyVariantType> _value;

    /* Single set of all group members across all groups */
    static std::set<std::string> _allMembers;

    /**
     * @brief Parse and set the members list
     *
     * @param[in] jsonObj - JSON object for the group
     *
     * Sets the list of dbus paths making up the members of the group
     */
    void setMembers(const json& jsonObj);

    /**
     * @brief Parse and set the service name(OPTIONAL)
     *
     * @param[in] jsonObj - JSON object for the group
     *
     * Sets the service name serving the members. It is recommended this service
     * name be provided for a group containing members served by the fan control
     * application itself, otherwise they may not be mapped correctly into any
     * configured events.
     */
    void setService(const json& jsonObj);
};

} // namespace phosphor::fan::control::json
