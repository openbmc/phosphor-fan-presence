/**
 * Copyright © 2022 IBM Corporation
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
 *    "default_floor": 2000,
 *    "fan_floors": [
 *        {
 *        "key": 27,
 *        "floor_offset_parameter": "floor_27_offset",
 *        "default_floor": 3000,
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
 *     to set the fan floor to, but first apply the floor offset provided
 *     by the parameter in the 'floor_offset_parameter' field, if it present.
 *   - If any group check results doesn't end in a match being found, then
 *     the default floor will be set.
 *
 * There are up to 3 default value values that may be used when a regular
 * floor value can't be calculated:
 *   1. Optional default floor at the key group level
 *     - Chosen when a floor wasn't found in any floor groups
 *   2. Optional default floor at the action level
 *     - Chosen when there isn't a key table found for the key value,
 *       or 1. above occurred but that default floor wasn't supplied.
 *   3. The default floor in the zone config
 *     - Chosen when when 2. would be used, but it wasn't supplied
 *
 * This action can also have a condition specified where a group property
 * must either match or not match a given value to determine if the
 * action should run or not.  This requires the following in the JSON:
 *    "condition_group": The group name
 *       - As of now, group must just have a single member.
 *    "condition_op": Either "equal" or "not_equal"
 *    "condition_value": The value to check against
 *
 * This allows completely separate mapped_floor actions to run based on
 * the value of a D-bus property - i.e. it allows multiple floor tables.
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
     * @brief Parse and set the default floor value
     *
     * @param[in] jsonObj - JSON object for the action
     */
    void setDefaultFloor(const json& jsonObj);

    /**
     * @brief Parses and sets the floor group data members
     *
     * @param[in] jsonObj - JSON object for the action
     */
    void setFloorTable(const json& jsonObj);

    /**
     * @brief Parse and set the conditions
     *
     * @param jsonObj - JSON object for the action
     */
    void setCondition(const json& jsonObj);

    /**
     * @brief Applies the offset in offsetParameter to the
     *        value passed in.
     *
     * If offsetParameter is empty then no offset will be
     * applied.
     *
     * Note: The offset may be negative.
     *
     * @param[in] floor - The floor to apply offset to
     * @param[in] offsetParameter - The floor offset parameter
     *
     * @return uint64_t - The new floor value
     */
    uint64_t applyFloorOffset(uint64_t floor,
                              const std::string& offsetParameter) const;

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
     * @return optional<PropertyVariantType> - The value, or std::nullopt
     */
    std::optional<PropertyVariantType> getMaxGroupValue(const Group& group);

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

    /**
     * @brief Checks if the condition is met, if there is one.
     *
     * @return bool - False if there is a condition and it
     *                isn't met, true otherwise.
     */
    bool meetsCondition();

    /* Key group pointer */
    const Group* _keyGroup;

    /* condition group pointer */
    const Group* _conditionGroup = nullptr;

    /* Condition value */
    PropertyVariantType _conditionValue;

    /* Condition operation */
    std::string _conditionOp;

    /* Optional default floor value for the action */
    std::optional<uint64_t> _defaultFloor;

    using FloorEntry = std::tuple<PropertyVariantType, uint64_t>;

    struct FloorGroup
    {
        std::variant<const Group*, std::string> groupOrParameter;
        std::vector<FloorEntry> floorEntries;
    };

    struct FanFloors
    {
        PropertyVariantType keyValue;
        std::string offsetParameter;
        std::optional<uint64_t> defaultFloor;
        std::vector<FloorGroup> floorGroups;
    };

    /* The fan floors action data, loaded from JSON */
    std::vector<FanFloors> _fanFloors;
};

} // namespace phosphor::fan::control::json
