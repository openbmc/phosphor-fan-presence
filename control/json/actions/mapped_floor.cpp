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
#include "mapped_floor.hpp"

#include "../manager.hpp"
#include "../zone.hpp"
#include "group.hpp"
#include "sdeventplus.hpp"

#include <fmt/format.h>

#include <nlohmann/json.hpp>

#include <algorithm>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

template <typename T>
uint64_t addFloorOffset(uint64_t floor, T offset, const std::string& actionName)
{
    if constexpr (!std::is_arithmetic_v<T>)
    {
        throw std::runtime_error("Invalid variant type in addFloorOffset");
    }

    auto newFloor = static_cast<T>(floor) + offset;
    if (newFloor < 0)
    {
        log<level::ERR>(
            fmt::format("{}: Floor offset of {} resulted in negative floor",
                        actionName, offset)
                .c_str());
        return floor;
    }

    return static_cast<uint64_t>(newFloor);
}

MappedFloor::MappedFloor(const json& jsonObj,
                         const std::vector<Group>& groups) :
    ActionBase(jsonObj, groups)
{
    setKeyGroup(jsonObj);
    setFloorTable(jsonObj);
    setDefaultFloor(jsonObj);
    setCondition(jsonObj);
}

const Group* MappedFloor::getGroup(const std::string& name)
{
    auto groupIt =
        find_if(_groups.begin(), _groups.end(),
                [name](const auto& group) { return name == group.getName(); });

    if (groupIt == _groups.end())
    {
        throw ActionParseError{
            ActionBase::getName(),
            fmt::format("Group name {} is not a valid group", name)};
    }

    return &(*groupIt);
}

void MappedFloor::setKeyGroup(const json& jsonObj)
{
    if (!jsonObj.contains("key_group"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required 'key_group' entry"};
    }
    _keyGroup = getGroup(jsonObj["key_group"].get<std::string>());
}

void MappedFloor::setDefaultFloor(const json& jsonObj)
{
    if (jsonObj.contains("default_floor"))
    {
        _defaultFloor = jsonObj["default_floor"].get<uint64_t>();
    }
}

void MappedFloor::setFloorTable(const json& jsonObj)
{
    if (!jsonObj.contains("fan_floors"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing fan_floors JSON entry"};
    }

    const auto& fanFloors = jsonObj.at("fan_floors");

    for (const auto& floors : fanFloors)
    {
        if (!floors.contains("key") || !floors.contains("floors"))
        {
            throw ActionParseError{
                ActionBase::getName(),
                "Missing key or floors entries in actions/fan_floors JSON"};
        }

        FanFloors ff;
        ff.keyValue = getJsonValue(floors["key"]);

        if (floors.contains("floor_offset_parameter"))
        {
            ff.offsetParameter =
                floors["floor_offset_parameter"].get<std::string>();
        }

        if (floors.contains("default_floor"))
        {
            ff.defaultFloor = floors["default_floor"].get<uint64_t>();
        }

        for (const auto& groupEntry : floors["floors"])
        {
            if ((!groupEntry.contains("group") &&
                 !groupEntry.contains("parameter")) ||
                !groupEntry.contains("floors"))
            {
                throw ActionParseError{
                    ActionBase::getName(),
                    "Missing group, parameter, or floors entries in "
                    "actions/fan_floors/floors JSON"};
            }

            FloorGroup fg;
            if (groupEntry.contains("group"))
            {
                fg.groupOrParameter =
                    getGroup(groupEntry["group"].get<std::string>());
            }
            else
            {
                fg.groupOrParameter =
                    groupEntry["parameter"].get<std::string>();
            }

            for (const auto& floorEntry : groupEntry["floors"])
            {
                if (!floorEntry.contains("value") ||
                    !floorEntry.contains("floor"))
                {

                    throw ActionParseError{
                        ActionBase::getName(),
                        "Missing value or floor entries in "
                        "actions/fan_floors/floors/floors JSON"};
                }

                auto value = getJsonValue(floorEntry["value"]);
                auto floor = floorEntry["floor"].get<uint64_t>();

                fg.floorEntries.emplace_back(std::move(value),
                                             std::move(floor));
            }

            ff.floorGroups.push_back(std::move(fg));
        }

        _fanFloors.push_back(std::move(ff));
    }
}

void MappedFloor::setCondition(const json& jsonObj)
{
    // condition_group, condition_value, and condition_op
    // are optional, though they must show up together.
    // Assume if condition_group is present then they all
    // must be.
    if (!jsonObj.contains("condition_group"))
    {
        return;
    }

    _conditionGroup = getGroup(jsonObj["condition_group"].get<std::string>());

    if (_conditionGroup->getMembers().size() != 1)
    {
        throw ActionParseError{
            ActionBase::getName(),
            fmt::format("condition_group {} must only have 1 member",
                        _conditionGroup->getName())};
    }

    if (!jsonObj.contains("condition_value"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required 'condition_value' entry in "
                               "mapped_floor action"};
    }

    _conditionValue = getJsonValue(jsonObj["condition_value"]);

    if (!jsonObj.contains("condition_op"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required 'condition_op' entry in "
                               "mapped_floor action"};
    }

    _conditionOp = jsonObj["condition_op"].get<std::string>();

    if ((_conditionOp != "equal") && (_conditionOp != "not_equal"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Invalid 'condition_op' value in "
                               "mapped_floor action"};
    }
}

/**
 * @brief Converts the variant to a double if it's a
 *        int32_t or int64_t.
 */
void tryConvertToDouble(PropertyVariantType& value)
{
    std::visit(
        [&value](auto&& val) {
            using V = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<int32_t, V> ||
                          std::is_same_v<int64_t, V>)
            {
                value = static_cast<double>(val);
            }
        },
        value);
}

std::optional<PropertyVariantType>
    MappedFloor::getMaxGroupValue(const Group& group)
{
    std::optional<PropertyVariantType> max;
    bool checked = false;

    for (const auto& member : group.getMembers())
    {
        try
        {
            auto value = Manager::getObjValueVariant(
                member, group.getInterface(), group.getProperty());

            // Only allow a group to have multiple members if it's numeric.
            // Unlike std::is_arithmetic, bools are not considered numeric
            // here.
            if (!checked && (group.getMembers().size() > 1))
            {
                std::visit(
                    [&group, this](auto&& val) {
                        using V = std::decay_t<decltype(val)>;
                        if constexpr (!std::is_same_v<double, V> &&
                                      !std::is_same_v<int32_t, V> &&
                                      !std::is_same_v<int64_t, V>)
                        {
                            throw std::runtime_error{fmt::format(
                                "{}: Group {} has more than one member but "
                                "isn't numeric",
                                ActionBase::getName(), group.getName())};
                        }
                    },
                    value);
                checked = true;
            }

            if (max && (value > max))
            {
                max = value;
            }
            else if (!max)
            {
                max = value;
            }
        }
        catch (const std::out_of_range& e)
        {
            // Property not there, continue on
        }
    }

    if (max)
    {
        tryConvertToDouble(*max);
    }

    return max;
}

bool MappedFloor::meetsCondition()
{
    if (!_conditionGroup)
    {
        return true;
    }

    bool meets = false;

    // setCondition() also checks these
    assert(_conditionGroup->getMembers().size() == 1);
    assert((_conditionOp == "equal") || (_conditionOp == "not_equal"));

    const auto& member = _conditionGroup->getMembers()[0];

    try
    {
        auto value =
            Manager::getObjValueVariant(member, _conditionGroup->getInterface(),
                                        _conditionGroup->getProperty());

        if ((_conditionOp == "equal") && (value == _conditionValue))
        {
            meets = true;
        }
        else if ((_conditionOp == "not_equal") && (value != _conditionValue))
        {
            meets = true;
        }
    }
    catch (const std::out_of_range& e)
    {
        // Property not there, so consider it failing the 'equal'
        // condition and passing the 'not_equal' condition.
        if (_conditionOp == "equal")
        {
            meets = false;
        }
        else // not_equal
        {
            meets = true;
        }
    }

    return meets;
}

void MappedFloor::run(Zone& zone)
{
    if (!meetsCondition())
    {
        // Make sure this no longer has a floor hold
        if (zone.hasFloorHold(getUniqueName()))
        {
            zone.setFloorHold(getUniqueName(), 0, false);
        }
        return;
    }

    std::optional<uint64_t> newFloor;

    auto keyValue = getMaxGroupValue(*_keyGroup);
    if (!keyValue)
    {
        auto floor = _defaultFloor ? *_defaultFloor : zone.getDefaultFloor();
        zone.setFloorHold(getUniqueName(), floor, true);
        return;
    }

    for (const auto& floorTable : _fanFloors)
    {
        // First, find the floorTable entry to use based on the key value.
        auto tableKeyValue = floorTable.keyValue;

        // Convert numeric values from the JSON to doubles so they can
        // be compared to values coming from D-Bus.
        tryConvertToDouble(tableKeyValue);

        // The key value from D-Bus must be less than the value
        // in the table for this entry to be valid.
        if (*keyValue >= tableKeyValue)
        {
            continue;
        }

        // Now check each group in the tables
        for (const auto& [groupOrParameter, floorGroups] :
             floorTable.floorGroups)
        {
            std::optional<PropertyVariantType> propertyValue;

            if (std::holds_alternative<std::string>(groupOrParameter))
            {
                propertyValue = Manager::getParameter(
                    std::get<std::string>(groupOrParameter));
                if (propertyValue)
                {
                    tryConvertToDouble(*propertyValue);
                }
                else
                {
                    // If the parameter isn't there, then don't use
                    // this floor table
                    log<level::DEBUG>(
                        fmt::format("{}: Parameter {} specified in the JSON "
                                    "could not be found",
                                    ActionBase::getName(),
                                    std::get<std::string>(groupOrParameter))
                            .c_str());
                    continue;
                }
            }
            else
            {
                propertyValue =
                    getMaxGroupValue(*std::get<const Group*>(groupOrParameter));
            }

            std::optional<uint64_t> floor;
            if (propertyValue)
            {
                // Do either a <= or an == check depending on the data type
                // to get the floor value based on this group.
                for (const auto& [tableValue, tableFloor] : floorGroups)
                {
                    PropertyVariantType value{tableValue};
                    tryConvertToDouble(value);

                    if (std::holds_alternative<double>(*propertyValue))
                    {
                        if (*propertyValue <= value)
                        {
                            floor = tableFloor;
                            break;
                        }
                    }
                    else if (*propertyValue == value)
                    {
                        floor = tableFloor;
                        break;
                    }
                }
            }

            // No floor found in this group, use a default floor for now but
            // let keep going in case it finds a higher one.
            if (!floor)
            {
                if (floorTable.defaultFloor)
                {
                    floor = *floorTable.defaultFloor;
                }
                else if (_defaultFloor)
                {
                    floor = *_defaultFloor;
                }
                else
                {
                    floor = zone.getDefaultFloor();
                }
            }

            // Keep track of the highest floor value found across all
            // entries/groups
            if ((newFloor && (floor > *newFloor)) || !newFloor)
            {
                newFloor = floor;
            }
        }

        // if still no floor, use the default one from the floor table if
        // there
        if (!newFloor && floorTable.defaultFloor)
        {
            newFloor = floorTable.defaultFloor.value();
        }

        if (newFloor)
        {
            *newFloor = applyFloorOffset(*newFloor, floorTable.offsetParameter);
        }

        // Valid key value for this entry, so done
        break;
    }

    if (!newFloor)
    {
        newFloor = _defaultFloor ? *_defaultFloor : zone.getDefaultFloor();
    }

    zone.setFloorHold(getUniqueName(), *newFloor, true);
}

uint64_t MappedFloor::applyFloorOffset(uint64_t floor,
                                       const std::string& offsetParameter) const
{
    if (!offsetParameter.empty())
    {
        auto offset = Manager::getParameter(offsetParameter);
        if (offset)
        {
            if (std::holds_alternative<int32_t>(*offset))
            {
                return addFloorOffset(floor, std::get<int32_t>(*offset),
                                      getUniqueName());
            }
            else if (std::holds_alternative<int64_t>(*offset))
            {
                return addFloorOffset(floor, std::get<int64_t>(*offset),
                                      getUniqueName());
            }
            else if (std::holds_alternative<double>(*offset))
            {
                return addFloorOffset(floor, std::get<double>(*offset),
                                      getUniqueName());
            }
            else
            {
                throw std::runtime_error(
                    "Invalid data type in floor offset parameter ");
            }
        }
    }

    return floor;
}

} // namespace phosphor::fan::control::json
