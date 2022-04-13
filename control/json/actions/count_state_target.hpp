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
 * @class CountStateTarget - Action to set a target when members are at a state
 *
 * Sets the fans to a configured target when a number of members within the
 * group are at a configured state. Once the number of members at the given
 * state falls below the configured count, active fan target changes are
 * allowed.
 */
class CountStateTarget :
    public ActionBase,
    public ActionRegister<CountStateTarget>
{
  public:
    /* Name of this action */
    static constexpr auto name = "count_state_before_target";

    CountStateTarget() = delete;
    CountStateTarget(const CountStateTarget&) = delete;
    CountStateTarget(CountStateTarget&&) = delete;
    CountStateTarget& operator=(const CountStateTarget&) = delete;
    CountStateTarget& operator=(CountStateTarget&&) = delete;
    ~CountStateTarget() = default;

    /**
     * @brief Set target when a number of members are at a given state
     *
     * @param[in] jsonObj - JSON configuration of this action
     * @param[in] groups - Groups of dbus objects the action uses
     */
    CountStateTarget(const json& jsonObj, const std::vector<Group>& groups);

    /**
     * @brief Run the action
     *
     * Counts the number of members within a group are equal to a given state
     * and when the number of members are at or above the given state, the fans
     * within the zone are set to the configured target. The fans are held at
     * the configured target until the number of members equal to the given
     * state fall below the provided count.
     *
     * @param[in] zone - Zone to run the action on
     */
    void run(Zone& zone) override;

  private:
    /* Number of group members */
    size_t _count;

    /* State the members must be at to set the target */
    PropertyVariantType _state;

    /* Target for this action */
    uint64_t _target;

    /* Unique id of this action */
    size_t _id;

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
};

} // namespace phosphor::fan::control::json
