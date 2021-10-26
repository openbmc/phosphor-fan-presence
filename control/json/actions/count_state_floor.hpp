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
 * @class CountStateFloor - Action to set a floor when members are at a state
 *
 * Sets the fans to a configured floor when a number of members within the
 * group are at a configured state. Once the number of members at the given
 * state falls below the configured count, the floor hold is released.
 *
 * Example JSON:
 *    {
 *      "name": "count_state_floor",
 *      "count": 1,
 *      "state": false,
 *      "floor": 5000
 *    }
 */
class CountStateFloor :
    public ActionBase,
    public ActionRegister<CountStateFloor>
{
  public:
    /* Name of this action */
    static constexpr auto name = "count_state_floor";

    CountStateFloor() = delete;
    CountStateFloor(const CountStateFloor&) = delete;
    CountStateFloor(CountStateFloor&&) = delete;
    CountStateFloor& operator=(const CountStateFloor&) = delete;
    CountStateFloor& operator=(CountStateFloor&&) = delete;
    ~CountStateFloor() = default;

    /**
     * @brief Set floor when a number of members are at a given state
     *
     * @param[in] jsonObj - JSON configuration of this action
     * @param[in] groups - Groups of dbus objects the action uses
     */
    CountStateFloor(const json& jsonObj, const std::vector<Group>& groups);

    /**
     * @brief Run the action
     *
     * Counts the number of members within a group are equal to a given state
     * and when the number of members are at or above the given state, the
     * zone is set to the configured floor, with a hold.  The hold is released
     * when the number of members goes below the configured count.
     *
     * @param[in] zone - Zone to run the action on
     */
    void run(Zone& zone) override;

  private:
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
     * Sets the state for each member to equal to set the floor
     */
    void setState(const json& jsonObj);

    /**
     * @brief Parse and set the floor
     *
     * @param[in] jsonObj - JSON object for the action
     *
     * Sets the floor to use when running the action
     */
    void setFloor(const json& jsonObj);

    /* Number of group members */
    size_t _count;

    /* State the members must be at to set the floor */
    PropertyVariantType _state;

    /* Floor for this action */
    uint64_t _floor;
};

} // namespace phosphor::fan::control::json
