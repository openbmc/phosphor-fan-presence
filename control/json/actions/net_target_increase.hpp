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

#include <optional>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

/**
 * @class NetTargetIncrease - Action to determine the net target increase to
 * request
 *
 * Calculates the net target increase to be requested based on the value of each
 * property given within a group. The net target increase is the maximum delta
 * determined from all of the properties of the group. This net target increase
 * is the increase change that's requested to the current target of a zone.
 */
class NetTargetIncrease :
    public ActionBase,
    public ActionRegister<NetTargetIncrease>
{
  public:
    /* Name of this action */
    static constexpr auto name = "set_net_increase_target";

    NetTargetIncrease() = delete;
    NetTargetIncrease(const NetTargetIncrease&) = delete;
    NetTargetIncrease(NetTargetIncrease&&) = delete;
    NetTargetIncrease& operator=(const NetTargetIncrease&) = delete;
    NetTargetIncrease& operator=(NetTargetIncrease&&) = delete;
    ~NetTargetIncrease() = default;

    /**
     * @brief Determine/Set the net target increase
     *
     * @param[in] jsonObj - JSON configuration of this action
     * @param[in] groups - Groups of dbus objects the action uses
     */
    NetTargetIncrease(const json& jsonObj, const std::vector<Group>& groups);

    /**
     * @brief Run the action
     *
     * Determines the net target increase delta to be requested based on the
     * property values of the group of dbus objects and requests this target
     * increase on the zone. The property values of the group is compared to
     * the configured state value to determine if an increase delta should be
     * requested. For boolean & string values, the configured delta is
     * requested when a property of the group equals the configured state. For
     * integer & double values, the configured delta is multiplied by the
     * difference in property value and the configured state resulting in a net
     * target increase for each group member. After all members of the group of
     * dbus objects are processed, the maximum net target increase calculated is
     * requested on the zone.
     *
     * The state value can be specified as number in the JSON, or as a Manager
     * parameter that another action will have set.
     *
     * @param[in] zone - Zone to run the action on
     */
    void run(Zone& zone) override;

  private:
    /* State the members must be at to increase the target */
    PropertyVariantType _state;

    /**
     * The Manager parameter to use to get the state value if that
     * was the method specified in the JSON.
     */
    std::string _stateParameter;

    /* Increase delta for this action */
    uint64_t _delta;

    /**
     * @brief Parse and set the state
     *
     * @param[in] jsonObj - JSON object for the action
     *
     * Sets the state to compare members to
     */
    void setState(const json& jsonObj);

    /**
     * @brief Parse and set the delta
     *
     * @param[in] jsonObj - JSON object for the action
     *
     * Sets the increase delta to use when running the action
     */
    void setDelta(const json& jsonObj);
};

} // namespace phosphor::fan::control::json
