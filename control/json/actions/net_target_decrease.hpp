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
 * @class NetTargetDecrease - Action to determine the net target decrease to
 * request
 *
 * Calculates the net target decrease to be requested based on the value of each
 * property given within a group. The net target decrease is the minimum delta
 * determined from all of the properties of the group. This net target decrease
 * is the decrease change that's requested to the current target of a zone.
 */
class NetTargetDecrease :
    public ActionBase,
    public ActionRegister<NetTargetDecrease>
{
  public:
    /* Name of this action */
    static constexpr auto name = "set_net_decrease_target";

    NetTargetDecrease() = delete;
    NetTargetDecrease(const NetTargetDecrease&) = delete;
    NetTargetDecrease(NetTargetDecrease&&) = delete;
    NetTargetDecrease& operator=(const NetTargetDecrease&) = delete;
    NetTargetDecrease& operator=(NetTargetDecrease&&) = delete;
    ~NetTargetDecrease() = default;

    /**
     * @brief Determine/Set the net target decrease
     *
     * @param[in] jsonObj - JSON configuration of this action
     * @param[in] groups - Groups of dbus objects the action uses
     */
    NetTargetDecrease(const json& jsonObj, const std::vector<Group>& groups);

    /**
     * @brief Run the action
     *
     * Determines the net target decrease delta to be requested based on the
     * property values of the group of dbus objects and requests this target
     * decrease on the zone. The property values of the group is compared to
     * the configured state value to determine if an decrease delta should be
     * requested. For boolean & string values, the configured delta is
     * requested when a property of the group equals the configured state. For
     * integer & double values, the configured delta is multiplied by the
     * difference in property value and the configured state resulting in a net
     * target decrease for each group member. After all members of the group of
     * dbus objects are processed, the minimum net target decrease calculated is
     * requested on the zone.
     *
     * The state value can be specified in the JSON, or as a Manager parameter
     * that another action will have set.
     *
     * @param[in] zone - Zone to run the action on
     */
    void run(Zone& zone) override;

  private:
    /* State the members must be at to decrease the target */
    PropertyVariantType _state;

    /**
     * The Manager parameter to use to get the state value if that
     * was the method specified in the JSON.
     */
    std::string _stateParameter;

    /* Decrease delta for this action */
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
     * Sets the decrease delta to use when running the action
     */
    void setDelta(const json& jsonObj);
};

} // namespace phosphor::fan::control::json
