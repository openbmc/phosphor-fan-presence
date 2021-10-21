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
 * @class MissingOwnerTarget - Action to set a target when an owner is missing
 *
 * Sets the fans to a configured target when any service owner associated to the
 * group is missing. Once all services are functional and providing all the
 * group data again, active fan target changes are allowed.
 */
class MissingOwnerTarget :
    public ActionBase,
    public ActionRegister<MissingOwnerTarget>
{
  public:
    /* Name of this action */
    static constexpr auto name = "set_target_on_missing_owner";

    MissingOwnerTarget() = delete;
    MissingOwnerTarget(const MissingOwnerTarget&) = delete;
    MissingOwnerTarget(MissingOwnerTarget&&) = delete;
    MissingOwnerTarget& operator=(const MissingOwnerTarget&) = delete;
    MissingOwnerTarget& operator=(MissingOwnerTarget&&) = delete;
    ~MissingOwnerTarget() = default;

    /**
     * @brief Set target on an owner missing
     *
     * @param[in] jsonObj - JSON configuration of this action
     * @param[in] groups - Groups of dbus objects the action uses
     */
    MissingOwnerTarget(const json& jsonObj, const std::vector<Group>& groups);

    /**
     * @brief Run the action
     *
     * Updates the services of the group, then determines if any of the
     * services hosting the members of the group are not owned on dbus
     * resulting in the zone's target being set/held at the configured target.
     *
     * @param[in] zone - Zone to run the action on
     */
    void run(Zone& zone) override;

  private:
    /* Target for this action */
    uint64_t _target;

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
