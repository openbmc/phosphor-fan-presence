/**
 * Copyright © 2022 Ampere Computing
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

#include "../utils/modifier.hpp"
#include "../zone.hpp"
#include "action.hpp"
#include "group.hpp"

#include <nlohmann/json.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

/**
 * @class class TargetFromGroupMax : - Action to set target of Zone
 * to a value corresponding to the maximum value from group's member
 * properties. The mapping is according to the configurable map.
 *
 * If there are more than one group using this action, the maximum
 * speed derived from the mapping of all groups will be set to target.
 *
 * For example:
 *
 *  {
      "name": "target_from_group_max",
      "groups": [
            {
              "name": "zone0_ambient",
              "interface": "xyz.openbmc_project.Sensor.Value",
              "property": { "name": "Value" }
            }
          ],
      "neg_hysteresis": 1,
      "pos_hysteresis": 0,
      "map": [
            { "value": 10.0, "target": 38.0 }
      ]
    }

 *
 * The above JSON will cause the action to read the property specified
 * in the group "zone0_ambient" from all members of the group, the change
 * in the group's members value will be checked against "neg_hysteresis"
 * and "pos_hysteresis" to decide if it is worth taking action.
 * "neg_hysteresis" is for the increasing case and "pos_hysteresis" is
 * for the decreasing case. The maximum property value in a group will be
 * mapped to the "map" to get the output "target". The updated "target"
 * value of each group will be stored in a static map with a key. The
 * maximum value from the static map will be used to set to the Zone's target.
 *
 */
class TargetFromGroupMax :
    public ActionBase,
    public ActionRegister<TargetFromGroupMax>
{
  public:
    /* Name of this action */
    static constexpr auto name = "target_from_group_max";

    TargetFromGroupMax() = delete;
    TargetFromGroupMax(const TargetFromGroupMax&) = delete;
    TargetFromGroupMax(TargetFromGroupMax&&) = delete;
    TargetFromGroupMax& operator=(const TargetFromGroupMax&) = delete;
    TargetFromGroupMax& operator=(TargetFromGroupMax&&) = delete;
    ~TargetFromGroupMax() = default;

    /**
     * @brief Constructor
     *
     * @param[in] jsonObj - JSON configuration of this action
     * @param[in] groups - Groups of dbus objects the action uses
     */
    TargetFromGroupMax(const json& jsonObj, const std::vector<Group>& groups);

    /**
     * @brief Reads a property value from the configured group,
     *        get the max, do mapping and get the target.
     *
     * @param[in] zone - Zone to run the action on
     */
    void run(Zone& zone) override;

  private:
    /*The previous maximum property value from group used for checking against
     * hysteresis*/
    uint64_t _prevGroupValue = 0;

    /*The table of maximum speed derived from each group using this action*/
    static std::map<size_t, uint64_t> _speedFromGroupsMap;

    /*The group index counter*/
    static size_t _groupIndexCounter;

    /*The Hysteresis parameters from config*/
    uint64_t _negHysteresis = 0;
    uint64_t _posHysteresis = 0;

    /*The group index from config*/
    size_t _groupIndex = 0;

    /*The mapping table from config*/
    std::map<uint64_t, uint64_t> _valueToSpeedMap;

    /**
     * @brief Read the hysteresis parameters from the JSON
     *
     * @param[in] jsonObj - JSON configuration of this action
     */
    void setHysteresis(const json& jsonObj);

    /**
     * @brief Set index for the group
     *
     * @param[in] jsonObj - JSON configuration of this action
     */
    void setIndex();

    /**
     * @brief Read the map from the JSON
     *
     * @param[in] jsonObj - JSON configuration of this action
     */
    void setMap(const json& jsonObj);

    /**
     * @brief Process through all groups of the event and return the maximum
     * property value
     *
     * @param[in] jsonObj - JSON configuration of this action
     */
    std::optional<PropertyVariantType> processGroups();
};

} // namespace phosphor::fan::control::json
