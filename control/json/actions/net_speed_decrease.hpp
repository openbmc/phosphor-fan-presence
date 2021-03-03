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
 * @class NetSpeedDecrease - Action to determine the net speed decrease to
 * request
 *
 * Calculates the net speed decrease to be requested based on the value of each
 * property given within a group. The net speed decrease is the minimum delta
 * determined from all of the properties of the group. This net speed decrease
 * is the decrease change that's requested to the current target of a zone.
 */
class NetSpeedDecrease :
    public ActionBase,
    public ActionRegister<NetSpeedDecrease>
{
  public:
    /* Name of this action */
    static constexpr auto name = "set_net_decrease_speed";

    NetSpeedDecrease() = delete;
    NetSpeedDecrease(const NetSpeedDecrease&) = delete;
    NetSpeedDecrease(NetSpeedDecrease&&) = delete;
    NetSpeedDecrease& operator=(const NetSpeedDecrease&) = delete;
    NetSpeedDecrease& operator=(NetSpeedDecrease&&) = delete;
    ~NetSpeedDecrease() = default;

    /**
     * @brief Determine/Set the net speed decrease
     *
     * @param[in] jsonObj - JSON configuration of this action
     */
    explicit NetSpeedDecrease(const json& jsonObj);

    /**
     * @brief Run the action
     *
     * Determines the net speed decrease delta to be requested based on the
     * property values of the group of dbus objects and requests this speed
     * decrease on the zone. The property values of the group is compared to
     * the configured state value to determine if an decrease delta should be
     * requested. For boolean & string values, the configured delta is
     * requested when a property of the group equals the configured state. For
     * integer & double values, the configured delta is multiplied by the
     * difference in property value and the configured state resulting in a net
     * speed decrease for each group member. After all members of the group of
     * dbus objects are processed, the minimum net speed decrease calculated is
     * requested on the zone.
     *
     * @param[in] zone - Zone to run the action on
     * @param[in] group - Group of dbus objects the action runs against
     */
    void run(Zone& zone, const Group& group) override;

  private:
    /* State the members must be at to decrease the speed */
    PropertyVariantType _state;

    /* decrease delta for this action */
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
