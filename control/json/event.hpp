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
#include "trigger_aliases.hpp"

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
 * to be run when a trigger occurs. The triggers and actions configured must be
 * available within the fan control application source.
 *
 * When no events exist, the configured fans are set to their corresponding
 * zone's `full_speed` value.
 */
class Event : public ConfigBase
{
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
     * @param[in] zones - Reference to the configured zones
     */
    Event(const json& jsonObj, Manager* mgr,
          std::map<configKey, std::unique_ptr<Zone>>& zones);

    /**
     * @brief Enable the event
     *
     * Performs the necessary tasks to enable the event such as enabling all the
     * event triggers, etc...
     */
    void enable();

    /**
     * @brief Call any power on triggers
     */
    void powerOn();

    /**
     * @brief Call any power off triggers
     */
    void powerOff();

    /**
     * @brief Clear all groups available for events
     */
    static void clearAllGroups()
    {
        allGroups.clear();
    }

    /**
     * @brief Set the groups that are available for events
     *
     * @param[in] groups - All groups available for events
     */
    static void
        setAllGroups(std::map<configKey, std::unique_ptr<Group>>&& groups)
    {
        allGroups = std::move(groups);
    }

    /**
     * @brief Load and/or return all groups available to be configured on events
     *
     * @param[in] loadGroups - Whether to load the groups or not
     *            (default is to load the groups if not already loaded)
     *
     * @return Groups available to be configured on events from `groups.json`
     */
    static std::map<configKey, std::unique_ptr<Group>>&
        getAllGroups(bool loadGroups = true);

    /**
     * @brief Parse group parameters and configure a group object
     *
     * @param[in] group - Group object to get configured
     * @param[in] jsonObj - JSON object for the group
     *
     * Configures a given group from a set of JSON configuration attributes
     */
    static void configGroup(Group& group, const json& jsonObj);

    /**
     * @brief Parse and set the event's groups(OPTIONAL)
     *
     * @param[in] jsonObj - JSON object for the event
     * @param[in] profiles - List of profiles to validate groups against
     * @param[out] groups - List of groups to be configured
     *
     * Sets the list of groups associated with the event
     */
    static void setGroups(const json& jsonObj,
                          const std::vector<std::string>& profiles,
                          std::vector<Group>& groups);

    /**
     * @brief Return the contained groups and actions as JSON
     *
     * @return json - A JSON object of groups and actions
     */
    json dump() const;

  private:
    /* The sdbusplus bus object */
    sdbusplus::bus_t& _bus;

    /* The event's manager */
    Manager* _manager;

    /* List of groups associated with the event */
    std::vector<Group> _groups;

    /* Reference to the configured zones */
    std::map<configKey, std::unique_ptr<Zone>>& _zones;

    /* List of actions for this event */
    std::vector<std::unique_ptr<ActionBase>> _actions;

    /* List of trigger type and enablement functions for this event */
    std::vector<std::tuple<std::string, trigger::enableTrigger>> _triggers;

    /* All groups available to be configred on events */
    static std::map<configKey, std::unique_ptr<Group>> allGroups;

    /**
     * @brief Parse and set the event's actions(OPTIONAL)
     *
     * @param[in] jsonObj - JSON object for the event
     *
     * Sets the list of actions to perform for the event
     */
    void setActions(const json& jsonObj);

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
