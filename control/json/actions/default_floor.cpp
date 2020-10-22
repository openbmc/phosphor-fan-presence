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
#include "default_floor.hpp"

#include "actions.hpp"
#include "functor.hpp"
#include "types.hpp"

#include <nlohmann/json.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

DefaultFloor::DefaultFloor(const json&) : ActionBase(DefaultFloor::name)
{
    // There are no additional JSON configuration parameters for this action
}

const Action DefaultFloor::getAction()
{
    return make_action(action::default_floor_on_missing_owner);
}

} // namespace phosphor::fan::control::json
