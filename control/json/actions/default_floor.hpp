/**
 * Copyright © 2020 IBM Corporation
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

#include <nlohmann/json.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

/**
 * @class DefaultFloor - Action to default the fan floor speed
 *
 * Sets the fan floor to the defined default fan floor speed when a
 * service associated to a given group has terminated. Once all services
 * are functional and providing the sensors, the fan floor is allowed
 * to be set normally again.
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

    explicit DefaultFloor(const json&);

    const Action getAction() override;
};

} // namespace phosphor::fan::control::json
