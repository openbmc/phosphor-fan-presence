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
 * @class DefaultFloor - Action to default the fan floor
 *
 * Sets the fan floor to the defined default fan floor when a service associated
 * to a given group has terminated. Once all services are functional and
 * providing the sensors, the fan floor is allowed to be set normally again.
 */
class DefaultFloor : public ActionBase, public ActionRegister<DefaultFloor>
{
  public:
    /* Name of this action */
    static constexpr auto name = "default_floor_on_missing_owner";

    DefaultFloor() = delete;
    DefaultFloor(const DefaultFloor&) = delete;
    DefaultFloor(DefaultFloor&&) = delete;
    DefaultFloor& operator=(const DefaultFloor&) = delete;
    DefaultFloor& operator=(DefaultFloor&&) = delete;
    ~DefaultFloor() = default;

    /**
     * @brief Default the fan floor
     *
     * @param[in] jsonObj - JSON configuration of this action
     * @param[in] groups - Groups of dbus objects the action uses
     */
    DefaultFloor(const json& jsonObj, const std::vector<Group>& groups);

    /**
     * @brief Run the action
     *
     * Updates the services of the group, then determines if any of the
     * services hosting the members of the group are not owned on dbus
     * resulting in the zone's floor being set/held at the default floor.
     *
     * @param[in] zone - Zone to run the action on
     */
    void run(Zone& zone) override;
};

} // namespace phosphor::fan::control::json
