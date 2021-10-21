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
 * @class RequestTargetBase - Action to set the requested target base
 *
 * Sets the base of a calculated requested target to the maximum value found
 * from the properties given within a group. The requested target base is what
 * the calculated requested target should be determined from when changing fan
 * targets. By default, the base of the next calculated requested target is the
 * current target of the zone. This action allows that base to be changed
 * according to the maximum value found from a given group of dbus objects.
 */
class RequestTargetBase :
    public ActionBase,
    public ActionRegister<RequestTargetBase>
{
  public:
    /* Name of this action */
    static constexpr auto name = "set_request_target_base_with_max";

    RequestTargetBase() = delete;
    RequestTargetBase(const RequestTargetBase&) = delete;
    RequestTargetBase(RequestTargetBase&&) = delete;
    RequestTargetBase& operator=(const RequestTargetBase&) = delete;
    RequestTargetBase& operator=(RequestTargetBase&&) = delete;
    ~RequestTargetBase() = default;

    /**
     * @brief Update the requested target base
     *
     * @param[in] jsonObj - JSON configuration of this action
     * @param[in] groups - Groups of dbus objects the action uses
     */
    RequestTargetBase(const json& jsonObj, const std::vector<Group>& groups);

    /**
     * @brief Run the action
     *
     * Determines the maximum value from the properties of the group of dbus
     * objects and sets the requested target base to this value. Only positive
     * integer or floating point types are supported as these are the only
     * valid types for a fan target to be based off of.
     *
     * @param[in] zone - Zone to run the action on
     */
    void run(Zone& zone) override;
};

} // namespace phosphor::fan::control::json
