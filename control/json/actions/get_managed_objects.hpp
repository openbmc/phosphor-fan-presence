/**
 * Copyright © 2021 IBM Corporation
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

#include "../zone.hpp"
#include "action.hpp"
#include "group.hpp"

#include <nlohmann/json.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

/**
 * @class GetManagedObjects
 *
 * This action adds the members of its groups to the object cache
 * by using Manager::addObjects() which calls the GetManagedObjects
 * D-Bus method to find and add the results.  When that is done,
 * it then runs any actions listed in the JSON.
 *
 * This allows an action to run with the latest values in the cache
 * without having to subscribe to propertiesChanged for them all.
 *
 * An example config is:
 *
 *    "actions": [
 *      {
 *        "name": "get_managed_objects",
 *        "groups": [
 *          {
 *            "name": "the_temps",
 *            "interface": "xyz.openbmc_project.Sensor.Value",
 *            "property": {
 *              "name": "Value"
 *            }
 *          }
 *        ],
 *        "actions": [
 *          {
 *            "name": "set_net_increase_target",
 *            "state": 30,
 *            "delta": 100
 *          }
 *        ]
 *     }
 *   ]
 **/
class GetManagedObjects :
    public ActionBase,
    public ActionRegister<GetManagedObjects>
{
  public:
    /* Name of this action */
    static constexpr auto name = "get_managed_objects";

    GetManagedObjects() = delete;
    GetManagedObjects(const GetManagedObjects&) = delete;
    GetManagedObjects(GetManagedObjects&&) = delete;
    GetManagedObjects& operator=(const GetManagedObjects&) = delete;
    GetManagedObjects& operator=(GetManagedObjects&&) = delete;
    ~GetManagedObjects() = default;

    /**
     * @brief Parse the JSON to set the members
     *
     * @param[in] jsonObj - JSON configuration of this action
     * @param[in] groups - Groups of dbus objects the action uses
     */
    GetManagedObjects(const json& jsonObj, const std::vector<Group>& groups);

    /**
     * @brief Run the action.
     *
     * @param[in] zone - Zone to run the action on
     */
    void run(Zone& zone) override;

    /**
     * @brief Set the zones on the action and the embedded actions
     *
     * @param[in] zones - Zones for the action and timer's actions
     *
     * Sets the zones on this action and the timer's actions to run against
     */
    void setZones(std::vector<std::reference_wrapper<Zone>>& zones) override;

  private:
    /**
     * @brief Parse and set the list of actions
     *
     * @param[in] jsonObj - JSON object for the action
     *
     * Sets the list of actions that is run when this action runs
     */
    void setActions(const json& jsonObj);

    /* List of actions to be called when this action runs */
    std::vector<std::unique_ptr<ActionBase>> _actions;
};

} // namespace phosphor::fan::control::json
