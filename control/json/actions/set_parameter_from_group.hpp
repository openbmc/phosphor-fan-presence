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
#include "util/modifier.hpp"

#include <nlohmann/json.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

/**
 * @class SetParameterFromGroup - Action to store a Parameter based
 * on a group property value.
 *
 * Sets a value in the Manager's parameter store based on the property
 * value of a group member.  The property value can be modified before
 * storing it if the JSON specifies a valid Modifier class expression.
 *
 * For example:
 *
 *  {
 *    "name": "set_parameter_from_group",
 *    "parameter_name": "proc_0_throttle_temp",
 *    "modifier": {
 *      "expression": "subtract",
 *      "value": 4
 *    }
 *  }
 *
 * The above JSON will cause the action to read the property specified
 * by the group, subtract 4 from it, and then write that value to the Manager
 * using the proc_0_throttle_temp name.
 *
 * Currently, only a single group is supported for this action, with that
 * group having a single member.
 *
 * See the Modifier class documentation for valid expresions.
 */
class SetParameterFromGroup :
    public ActionBase,
    public ActionRegister<SetParameterFromGroup>
{
  public:
    /* Name of this action */
    static constexpr auto name = "set_parameter_from_group";

    SetParameterFromGroup() = delete;
    SetParameterFromGroup(const SetParameterFromGroup&) = delete;
    SetParameterFromGroup(SetParameterFromGroup&&) = delete;
    SetParameterFromGroup& operator=(const SetParameterFromGroup&) = delete;
    SetParameterFromGroup& operator=(SetParameterFromGroup&&) = delete;
    ~SetParameterFromGroup() = default;

    /**
     * @brief Constructor
     *
     * @param[in] jsonObj - JSON configuration of this action
     * @param[in] groups - Groups of dbus objects the action uses
     */
    SetParameterFromGroup(const json& jsonObj,
                          const std::vector<Group>& groups);

    /**
     * @brief Reads a property value from the configured group,
     *        modifies it if specified, and then store the value
     *        in the Manager as a parameter.
     *
     * @param[in] zone - Zone to run the action on
     */
    void run(Zone& zone) override;

  private:
    /**
     * @brief Read the parameter name from the JSON
     *
     * @param[in] jsonObj - JSON configuration of this action
     */
    void setParameterName(const json& jsonObj);

    /**
     * @brief Read the optional modifier from the JSON
     *
     * @param[in] jsonObj - JSON configuration of this action
     */
    void setModifier(const json& jsonObj);

    /**
     * @brief The parameter name
     */
    std::string _name;

    /**
     * @brief The class used to modify the value
     *
     * Only created if a modifier is specified in the JSON.
     */
    std::unique_ptr<Modifier> _modifier;
};

} // namespace phosphor::fan::control::json
