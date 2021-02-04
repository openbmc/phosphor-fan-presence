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

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

/**
 * @class RequestSpeedBase - Action to set the requested speed base
 *
 * Sets the base of a calculated requested speed to the maximum value found
 * from the properties given within a group. The requested speed base is what
 * the calculated requested speed should be determined from when changing fan
 * speeds. By default, the base of the next calculated requested speed is the
 * current target speed of the zone. This action allows that base to be changed
 * according to the maximum value found from a given group of dbus objects.
 */
class RequestSpeedBase :
    public ActionBase,
    public ActionRegister<RequestSpeedBase>
{
  public:
    /* Name of this action */
    static constexpr auto name = "set_request_speed_base_with_max";

    RequestSpeedBase() = delete;
    RequestSpeedBase(const RequestSpeedBase&) = delete;
    RequestSpeedBase(RequestSpeedBase&&) = delete;
    RequestSpeedBase& operator=(const RequestSpeedBase&) = delete;
    RequestSpeedBase& operator=(RequestSpeedBase&&) = delete;
    ~RequestSpeedBase() = default;

    /**
     * @brief Update the requested speed base
     *
     * @param[in] jsonObj - JSON configuration of this action
     */
    explicit RequestSpeedBase(const json& jsonObj);

    /**
     * @brief Run the action
     *
     * Determines the maximum value from the properties of the group of dbus
     * objects and sets the requested speed base to this value. Only positive
     * integer or floating point types are supported as these are the only
     * valid types for a fan speed to be based off of.
     *
     * @param[in] zone - Zone to run the action on
     * @param[in] group - Group of dbus objects the action runs against
     */
    void run(Zone& zone, const Group& group) override;
};

} // namespace phosphor::fan::control::json
