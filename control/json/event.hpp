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

#include "action.hpp"
#include "config_base.hpp"
#include "group.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>

#include <memory>
#include <optional>
#include <tuple>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

/**
 * @class Event - Represents a configured fan control event
 *
 * Fan control events are optional, therefore the "events.json" file is
 * also optional. An event object can be used to enable a specific change to
 * how fan control should function. These events contain the configured
 * attributes that result in how fans are controlled within a system. Events
 * are made up of groups of sensors, triggers from those sensors, and actions
 * to be run when a trigger occurs. Events may also have a precondition that
 * must exist before the event is loaded into fan control. The triggers,
 * actions, and preconditions configured must be available within the fan
 * control application source.
 *
 * When no events exist, the configured fans are set to their corresponding
 * zone's `full_speed` value.
 */
class Event : public ConfigBase
{
    static constexpr auto precondName = 0;
    static constexpr auto precondGroups = 1;
    static constexpr auto precondEvents = 2;
    using Precondition =
        std::tuple<std::string,
                   std::vector<std::tuple<std::string, std::string, std::string,
                                          PropertyVariantType>>,
                   std::vector<Event>>;

  public:
    /* JSON file name for events */
    static constexpr auto confFileName = "events.json";

    Event() = delete;
    Event(const Event&) = delete;
    Event(Event&&) = delete;
    Event& operator=(const Event&) = delete;
    Event& operator=(Event&&) = delete;
    ~Event() = default;

    /**
     * Constructor
     * Parses and populates a configuration event from JSON object data
     *
     * @param[in] jsonObj - JSON object
     * @param[in] mgr - Manager of this event
     * @param[in] groups - Available groups that can be used
     * @param[in] zones - Reference to the configured zones
     */
    Event(const json& jsonObj, Manager* mgr,
          std::map<configKey, std::unique_ptr<Group>>& groups,
          std::map<configKey, std::unique_ptr<Zone>>& zones);

    /**
     * @brief Get the precondition
     *
     * @return The precondition details of the event
     */
    inline const auto& getPrecond() const
    {
        return _precond;
    }

    /**
     * @brief Get the groups
     *
     * @return List of groups associated with the event
     */
    inline const auto& getGroups() const
    {
        return _groups;
    }

    /**
     * @brief Get the actions
     *
     * @return List of actions to perform for the event
     */
    inline const auto& getActions() const
    {
        return _actions;
    }

  private:
    /* The sdbusplus bus object */
    sdbusplus::bus::bus& _bus;

    /* The event's manager */
    Manager* _manager;

    /* A precondition the event has in order to be enabled */
    Precondition _precond;

    /* List of groups associated with the event */
    std::vector<Group> _groups;

    /* Reference to the configured zones */
    std::map<configKey, std::unique_ptr<Zone>>& _zones;

    /* List of actions for this event */
    std::vector<std::unique_ptr<ActionBase>> _actions;

    /**
     * @brief Parse group parameters and configure a group object
     *
     * @param[in] group - Group object to get configured
     * @param[in] jsonObj - JSON object for the group
     *
     * Configures a given group from a set of JSON configuration attributes
     */
    void configGroup(Group& group, const json& jsonObj);

    /**
     * @brief Parse and set the event's precondition(OPTIONAL)
     *
     * @param[in] jsonObj - JSON object for the event
     * @param[in] groups - Available groups that can be used
     *
     * Sets the precondition of the event in order to be enabled
     */
    void setPrecond(const json& jsonObj,
                    std::map<configKey, std::unique_ptr<Group>>& groups);

    /**
     * @brief Parse and set the event's groups(OPTIONAL)
     *
     * @param[in] jsonObj - JSON object for the event
     * @param[in] groups - Available groups that can be used
     *
     * Sets the list of groups associated with the event
     */
    void setGroups(const json& jsonObj,
                   std::map<configKey, std::unique_ptr<Group>>& groups);

    /**
     * @brief Parse and set the event's actions(OPTIONAL)
     *
     * @param[in] jsonObj - JSON object for the event
     * @param[in] groups - Available groups that can be used
     *
     * Sets the list of actions to perform for the event
     */
    void setActions(const json& jsonObj,
                    std::map<configKey, std::unique_ptr<Group>>& groups);

    /**
     * @brief Parse and set the event's triggers
     *
     * @param[in] jsonObj - JSON object for the event
     *
     * Sets the list of triggers for the event
     */
    void setTriggers(const json& jsonObj);
};

} // namespace phosphor::fan::control::json
