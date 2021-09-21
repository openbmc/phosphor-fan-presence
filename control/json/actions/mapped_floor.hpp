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
 * @class MappedFloor - Action to set a fan floor based on ranges of
 *                      multiple sensor values.
 * For example, consider the following config:
 *
 *    {
 *    "name": "mapped_floor",
 *    "key_group": "ambient_temp",
 *    "fan_floors": [
 *        {
 *        "key": 27,
 *        "floors": [
 *          {
 *            "group": "altitude",
 *            "floors": [
 *               {
 *                 "value": 5000,
 *                 "floor": 4500
 *               }
 *            ]
 *          },
 *          {
 *            "group": "power_mode",
 *            "floors": [
 *               {
 *                 "value": "MaximumPerformance",
 *                 "floor": 5000
 *               }
 *            ]
 *          }
 *        ]
 *        }
 *      ]
 *    }
 *
 * When it runs, it will:
 *
 * 1. Evaluate the key_group
 *   - Find the max D-Bus property value (if numeric) of the member properties
 *     in this group.
 *   - Check it against each 'key' value in the fan_floor entries until
 *     the key_group property value < key value, so:
 *        max ambient temp < 27.
 *   - If the above check passes, the rest of that entry will be evaluated
 *     and then the action will be done.
 *
 * 2. Evaluate the group values in each floors array entry for this key value.
 *   - Find the max D-Bus property value (if numeric) of the member properties
 *     of this group - in this case 'altitude'.
 *   - Depending on the data type of that group, compare to the 'value' entry:
 *     - If numeric, check if the group's value is <= the 'value' one
 *     - Otherwise, check if it is ==
 *   - If that passes, save the value from the 'floor' entry and continue to
 *     the next entry in the floors array, which, if present, would specify
 *     another group to check.  In this case, 'power_mode'.  Repeat the above
 *     step.
 *   - After all the group compares are done, choose the largest floor value
 *     to set the fan floor to.  If any group check results doesn't end in
 *     a match being found, then the default floor will be set.
 *
 * Cases where the default floor will be set:
 *  - A table entry can't be found based on a key group's value.
 *  - A table entry can't be found based on a group's value.
 *  - A value can't be obtained for the 'key_group' D-Bus property group.
 *  - A value can't be obtained for any of the 'group' property groups.
 *  - A value is NaN, as no <, <=, or == checks would succeed.
 *
 * Other notes:
 *  - If a group has multiple members, they must be numeric or else
 *    the code will throw an exception.
 *
 *  - The group inside the floors array can also be a Manager parameter, so that
 *    this action can operate on a parameter value set by another action.
 *
 *    So instead of
 *            "group": "altitude",
 *    it can be:
 *            "parameter": "some_parameter"
 */

class MappedFloor : public ActionBase, public ActionRegister<MappedFloor>
{
  public:
    /* Name of this action */
    static constexpr auto name = "mapped_floor";

    MappedFloor() = delete;
    MappedFloor(const MappedFloor&) = delete;
    MappedFloor(MappedFloor&&) = delete;
    MappedFloor& operator=(const MappedFloor&) = delete;
    MappedFloor& operator=(MappedFloor&&) = delete;
    ~MappedFloor() = default;

    /**
     * @brief Parse the JSON to set the members
     *
     * @param[in] jsonObj - JSON configuration of this action
     * @param[in] groups - Groups of dbus objects the action uses
     */
    MappedFloor(const json& jsonObj, const std::vector<Group>& groups);

    /**
     * @brief Run the action.  See description above.
     *
     * @param[in] zone - Zone to run the action on
     */
    void run(Zone& zone) override;

  private:
    /**
     * @brief Parse and set the key group
     *
     * @param[in] jsonObj - JSON object for the action
     */
    void setKeyGroup(const json& jsonObj);

    /**
     * @brief Parses and sets the floor group data members
     *
     * @param[in] jsonObj - JSON object for the action
     */
    void setFloorTable(const json& jsonObj);

    /**
     * @brief Determines the maximum value of the property specified
     *        for the group of all members in the group.
     *
     * If not numeric, and more than one member, will throw an exception.
     * Converts numeric values to doubles so they can be compared later.
     *
     * If cannot get at least one valid value, returns std::nullopt.
     *
     * @param[in] group - The group to get the max value of
     *
     * @param[in] manager - The Manager object
     *
     * @return optional<PropertyVariantType> - The value, or std::nullopt
     */
    std::optional<PropertyVariantType> getMaxGroupValue(const Group& group,
                                                        const Manager& manager);

    /**
     * @brief Returns a pointer to the group object specified
     *
     * Throws ActionParseError if no group found
     *
     * @param[in] name - The group name
     *
     * @return const Group* - Pointer to the group
     */
    const Group* getGroup(const std::string& name);

    /* Key group pointer */
    const Group* _keyGroup;

    using FloorEntry = std::tuple<PropertyVariantType, uint64_t>;

    struct FloorGroup
    {
        std::variant<const Group*, std::string> groupOrParameter;
        std::vector<FloorEntry> floorEntries;
    };

    struct FanFloors
    {
        PropertyVariantType keyValue;
        std::vector<FloorGroup> floorGroups;
    };

    /* The fan floors action data, loaded from JSON */
    std::vector<FanFloors> _fanFloors;
};

} // namespace phosphor::fan::control::json
