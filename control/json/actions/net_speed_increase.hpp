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

#include "action.hpp"
#include "types.hpp"
#include "zone.hpp"

#include <nlohmann/json.hpp>

#include <optional>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

/**
 * @class NetSpeedIncrease - Action to determine the net speed increase to
 * request
 *
 * Calculates the net speed increase to be requested based on the value of each
 * property given within a group. The net speed increase is the maximum delta
 * determined from all of the properties of the group. This net speed increase
 * is the increase change that's requested to the current target of a zone.
 */
class NetSpeedIncrease :
    public ActionBase,
    public ActionRegister<NetSpeedIncrease>
{
  public:
    /* Name of this action */
    static constexpr auto name = "set_net_increase_speed";

    NetSpeedIncrease() = delete;
    NetSpeedIncrease(const NetSpeedIncrease&) = delete;
    NetSpeedIncrease(NetSpeedIncrease&&) = delete;
    NetSpeedIncrease& operator=(const NetSpeedIncrease&) = delete;
    NetSpeedIncrease& operator=(NetSpeedIncrease&&) = delete;
    ~NetSpeedIncrease() = default;

    /**
     * @brief Determine/Set the net speed increase
     *
     * @param[in] jsonObj - JSON configuration of this action
     */
    explicit NetSpeedIncrease(const json& jsonObj);

    /**
     * @brief Run the action
     *
     * Determines the net speed increase delta to be requested based on the
     * property values of the group of dbus objects and requests this speed
     * increase on the zone. The property values of the group is compared to
     * the configured state value to determine if an increase delta should be
     * requested. For boolean & string values, the configured delta is
     * requested when a property of the group equals the configured state. For
     * integer & double values, the configured delta is multiplied by the
     * difference in property value and the configured state resulting in a net
     * speed increase for each group member. After all members of the group of
     * dbus objects are processed, the maximum net speed increase calculated is
     * requested on the zone.
     *
     * @param[in] zone - Zone to run the action on
     * @param[in] group - Group of dbus objects the action runs against
     */
    void run(Zone& zone, const Group& group) override;

  private:
    /* State the members must be at to increase the speed */
    PropertyVariantType _state;

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
