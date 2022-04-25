/**
 * Copyright © 2022 IBM Corporation
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

#include "../utils/flight_recorder.hpp"
#include "../zone.hpp"
#include "config_base.hpp"
#include "group.hpp"

#include <fmt/format.h>

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

#include <algorithm>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <numeric>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using namespace phosphor::logging;

/**
 * @class ActionParseError - A parsing error exception
 *
 * A parsing error exception that can be used to terminate the application
 * due to not being able to successfully parse a configured action.
 */
class ActionParseError : public std::runtime_error
{
  public:
    ActionParseError() = delete;
    ActionParseError(const ActionParseError&) = delete;
    ActionParseError(ActionParseError&&) = delete;
    ActionParseError& operator=(const ActionParseError&) = delete;
    ActionParseError& operator=(ActionParseError&&) = delete;
    ~ActionParseError() = default;

    /**
     * @brief Action parsing error object
     *
     * When parsing an action from the JSON configuration, any critical
     * attributes that fail to be parsed for an action can throw an
     * ActionParseError exception to log the parsing failure details and
     * terminate the application.
     *
     * @param[in] name - Name of the action
     * @param[in] details - Additional details of the parsing error
     */
    ActionParseError(const std::string& name, const std::string& details) :
        std::runtime_error(
            fmt::format("Failed to parse action {} [{}]", name, details)
                .c_str())
    {}
};

/**
 * @brief Function used in creating action objects
 *
 * @param[in] jsonObj - JSON object for the action
 * @param[in] groups - Groups of dbus objects the action uses
 * @param[in] zones - Zones the action runs against
 *
 * Creates an action object given the JSON configuration, list of groups and
 * sets the zones the action should run against.
 */
template <typename T>
std::unique_ptr<T>
    createAction(const json& jsonObj, const std::vector<Group>& groups,
                 std::vector<std::reference_wrapper<Zone>>& zones)
{
    // Create the action and set its list of zones
    auto action = std::make_unique<T>(jsonObj, groups);
    action->setZones(zones);
    return action;
}

/**
 * @class ActionBase - Base action object
 *
 * Base class for fan control's event actions
 */
class ActionBase : public ConfigBase
{
  public:
    ActionBase() = delete;
    ActionBase(const ActionBase&) = delete;
    ActionBase(ActionBase&&) = delete;
    ActionBase& operator=(const ActionBase&) = delete;
    ActionBase& operator=(ActionBase&&) = delete;
    virtual ~ActionBase() = default;

    /**
     * @brief Base action object
     *
     * @param[in] jsonObj - JSON object containing name and any profiles
     * @param[in] groups - Groups of dbus objects the action uses
     *
     * All actions derived from this base action object must be given a name
     * that uniquely identifies the action. Optionally, a configured action can
     * have a list of explicit profiles it should be included in, otherwise
     * always include the action where no profiles are given.
     */
    ActionBase(const json& jsonObj, const std::vector<Group>& groups) :
        ConfigBase(jsonObj), _groups(groups),
        _uniqueName(getName() + "-" + std::to_string(_actionCount++))
    {}

    /**
     * @brief Get the groups configured on the action
     *
     * @return List of groups
     */
    inline const auto& getGroups() const
    {
        return _groups;
    }

    /**
     * @brief Set the zones the action is run against
     *
     * @param[in] zones - Zones the action runs against
     *
     * By default, the zones are set when the action object is created
     */
    virtual void setZones(std::vector<std::reference_wrapper<Zone>>& zones)
    {
        _zones = zones;
    }

    /**
     * @brief Add a zone to the list of zones the action is run against if its
     * not already there
     *
     * @param[in] zone - Zone to add
     */
    virtual void addZone(Zone& zone)
    {
        auto itZone =
            std::find_if(_zones.begin(), _zones.end(),
                         [&zone](std::reference_wrapper<Zone>& z) {
                             return z.get().getName() == zone.getName();
                         });
        if (itZone == _zones.end())
        {
            _zones.emplace_back(std::reference_wrapper<Zone>(zone));
        }
    }

    /**
     * @brief Run the action
     *
     * Run the action function associated to the derived action object
     * that performs a specific tasks on a zone configured by a user.
     *
     * @param[in] zone - Zone to run the action on
     */
    virtual void run(Zone& zone) = 0;

    /**
     * @brief Trigger the action to run against all of its zones
     *
     * This is the function used by triggers to run the actions against all the
     * zones that were configured for the action to run against.
     */
    void run()
    {
        std::for_each(_zones.begin(), _zones.end(),
                      [this](Zone& zone) { this->run(zone); });
    }

    /**
     * @brief Returns a unique name for the action.
     *
     * @return std::string - The name
     */
    const std::string& getUniqueName() const
    {
        return _uniqueName;
    }

    /**
     * @brief Set the name of the owning Event.
     *
     * Adds it to the unique name in parentheses. If desired,
     * the action child classes can do something else with it.
     *
     * @param[in] name - The event name
     */
    virtual void setEventName(const std::string& name)
    {
        if (!name.empty())
        {
            _uniqueName += '(' + name + ')';
        }
    }

    /**
     * @brief Dump the action as JSON
     *
     * For now just dump its group names
     *
     * @return json
     */
    json dump() const
    {
        json groups = json::array();
        std::for_each(_groups.begin(), _groups.end(),
                      [&groups](const auto& group) {
                          groups.push_back(group.getName());
                      });
        json output;
        output["groups"] = groups;
        return output;
    }

  protected:
    /**
     * @brief Logs a message to the flight recorder using
     *        the unique name of the action.
     *
     * @param[in] message - The message to log
     */
    void record(const std::string& message) const
    {
        FlightRecorder::instance().log(getUniqueName(), message);
    }

    /* Groups configured on the action */
    const std::vector<Group> _groups;

  private:
    /* Zones configured on the action */
    std::vector<std::reference_wrapper<Zone>> _zones;

    /* Unique name of the action.
     * It's just the name plus _actionCount at the time of action creation. */
    std::string _uniqueName;

    /* Running count of all actions */
    static inline size_t _actionCount = 0;
};

/**
 * @class ActionFactory - Factory for actions
 *
 * Factory that registers and retrieves actions based on a given name.
 */
class ActionFactory
{
  public:
    ActionFactory() = delete;
    ActionFactory(const ActionFactory&) = delete;
    ActionFactory(ActionFactory&&) = delete;
    ActionFactory& operator=(const ActionFactory&) = delete;
    ActionFactory& operator=(ActionFactory&&) = delete;
    ~ActionFactory() = default;

    /**
     * @brief Registers an action
     *
     * Registers an action as being available for configuration use. The action
     * is registered by its name and a function used to create the action
     * object. An action fails to be registered when another action of the same
     * name has already been registered. Actions with the same name would cause
     * undefined behavior, therefore are not allowed.
     *
     * Actions are registered prior to starting main().
     *
     * @param[in] name - Name of the action to register
     *
     * @return The action was registered, otherwise an exception is thrown.
     */
    template <typename T>
    static bool regAction(const std::string& name)
    {
        auto it = actions.find(name);
        if (it == actions.end())
        {
            actions[name] = &createAction<T>;
        }
        else
        {
            log<level::ERR>(
                fmt::format("Action '{}' is already registered", name).c_str());
            throw std::runtime_error("Actions with the same name found");
        }

        return true;
    }

    /**
     * @brief Gets a registered action's object
     *
     * Gets a registered action's object of a given name from the JSON
     * configuration data provided.
     *
     * @param[in] name - Name of the action to create/get
     * @param[in] jsonObj - JSON object for the action
     * @param[in] groups - Groups of dbus objects the action uses
     * @param[in] zones - Zones the action runs against
     *
     * @return Pointer to the action object.
     */
    static std::unique_ptr<ActionBase>
        getAction(const std::string& name, const json& jsonObj,
                  const std::vector<Group>& groups,
                  std::vector<std::reference_wrapper<Zone>>&& zones)
    {
        auto it = actions.find(name);
        if (it != actions.end())
        {
            return it->second(jsonObj, groups, zones);
        }
        else
        {
            // Construct list of available actions
            auto acts = std::accumulate(
                std::next(actions.begin()), actions.end(),
                actions.begin()->first, [](auto list, auto act) {
                    return std::move(list) + ", " + act.first;
                });
            log<level::ERR>(
                fmt::format("Action '{}' is not registered", name).c_str(),
                entry("AVAILABLE_ACTIONS=%s", acts.c_str()));
            throw std::runtime_error("Unsupported action name given");
        }
    }

  private:
    /* Map to store the available actions and their creation functions */
    static inline std::map<std::string,
                           std::function<std::unique_ptr<ActionBase>(
                               const json&, const std::vector<Group>&,
                               std::vector<std::reference_wrapper<Zone>>&)>>
        actions;
};

/**
 * @class ActionRegister - Registers an action class
 *
 * Base action registration class that is extended by an action object so
 * that action is registered and available for use.
 */
template <typename T>
class ActionRegister
{
  public:
    ActionRegister(const ActionRegister&) = delete;
    ActionRegister(ActionRegister&&) = delete;
    ActionRegister& operator=(const ActionRegister&) = delete;
    ActionRegister& operator=(ActionRegister&&) = delete;
    virtual ~ActionRegister() = default;
    ActionRegister()
    {
        // Templates instantiated when used, need to assign a value
        // here so the compiler doesnt remove it
        registered = true;
    }

  private:
    /* Register actions in the factory */
    static inline bool registered = ActionFactory::regAction<T>(T::name);
};

} // namespace phosphor::fan::control::json
