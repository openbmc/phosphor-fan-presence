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

#include "../zone.hpp"
#include "action.hpp"
#include "group.hpp"

#include <nlohmann/json.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

/**
 * @class OverrideFanTarget - Action to override fan targets
 *
 * This action locks fans at configured targets when the configured `count`
 * amount of fans meet criterion for the particular condition. A locked fan
 * maintains its override target until unlocked (or locked at a higher target).
 * Upon unlocking, it will either revert to temperature control or activate the
 * the next-highest target remaining in its list of locks.
 *
 * The following config will set all fans in the zone to a target of 9999 if
 * either fan has a properties_changed event where the Functional property goes
 * false. The count value of 1 means it only requires one fan, the state value
 * of false means Functional should go to false to be counted. The signal is
 * declared under the "triggers" section.

[
  {
    "name": "test",
    "groups": [
      { "name": "fan0 sensor inventory", "interface":
"xyz.openbmc_project.State.Decorator.OperationalStatus", "property": { "name":
"Functional" } }
    ],
    "triggers": [
      { "class": "init", "method": "get_properties" },
      { "class": "signal", "signal": "properties_changed" }
    ],
    "actions": [ {
      "name": "override_fan_target",
      "count": 1,
      "state": false,
      "fans": [ "fan0", "fan1", "fan2", "fan3" ],
      "target": 9999
    } ]
  }
]
 */

class OverrideFanTarget :
    public ActionBase,
    public ActionRegister<OverrideFanTarget>
{
  public:
    /* Name of this action */
    static constexpr auto name = "override_fan_target";

    OverrideFanTarget() = delete;
    OverrideFanTarget(const OverrideFanTarget&) = delete;
    OverrideFanTarget(OverrideFanTarget&&) = delete;
    OverrideFanTarget& operator=(const OverrideFanTarget&) = delete;
    OverrideFanTarget& operator=(OverrideFanTarget&&) = delete;
    ~OverrideFanTarget() = default;

    /**
     * @brief Set target when a number of members are at a given state
     *
     * @param[in] jsonObj - JSON configuration of this action
     * @param[in] groups - Groups of dbus objects the action uses
     */
    OverrideFanTarget(const json& jsonObj, const std::vector<Group>& groups);

    /**
     * @brief Run the action
     *
     * Counts the number of members within a group are at or above
     * the given state. The fans are held at the configured target
     * until the number of members equal to the given state falls
     * below the provided count.
     *
     * @param[in] zone - Zone to run the action on
     */
    void run(Zone& zone) override;

  private:
    /* action will be triggered when enough group members equal this state*/
    PropertyVariantType _state;

    /* how many group members required to be at _state to trigger action*/
    size_t _count;

    /* target for this action */
    uint64_t _target;

    /* store locked state to know when to unlock */
    bool _locked = false;

    /* which fans this action applies to */
    std::vector<std::string> _fans;

    /**
     * @brief lock all fans in this action
     *
     * @param[in] Zone - zone in which _fans are found
     *
     */
    void lockFans(Zone& zone);

    /**
     * @brief unlock all fans in this action
     *
     * @param[in] Zone - zone which _fans are found
     *
     */
    void unlockFans(Zone& zone);

    /**
     * @brief Parse and set the count
     *
     * @param[in] jsonObj - JSON object for the action
     *
     * Sets the number of members that must equal the state
     */
    void setCount(const json& jsonObj);

    /**
     * @brief Parse and set the state
     *
     * @param[in] jsonObj - JSON object for the action
     *
     * Sets the state for each member to equal to set the target
     */
    void setState(const json& jsonObj);

    /**
     * @brief Parse and set the target
     *
     * @param[in] jsonObj - JSON object for the action
     *
     * Sets the target to use when running the action
     */
    void setTarget(const json& jsonObj);

    /**
     * @brief Parse and set the fans
     *
     * @param[in] jsonObj - JSON object for the action
     *
     * Sets the fan list the action applies to
     */
    void setFans(const json& jsonObj);
};

} // namespace phosphor::fan::control::json
