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

MappedFloor::MappedFloor(const json& jsonObj,
                         const std::vector<Group>& groups) :
    ActionBase(jsonObj, groups),
    _ownerTimer(phosphor::fan::util::SDEventPlus::getEvent(),
                std::bind(&MappedFloor::ownerTimerExpired, this))
{
    setKeyGroup(jsonObj);
    setFloorTable(jsonObj);
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

        for (const auto& groupEntry : floors["floors"])
        {
            if (!groupEntry.contains("group") || !groupEntry.contains("floors"))
            {
                throw ActionParseError{ActionBase::getName(),
                                       "Missing group or floors entries in "
                                       "actions/fan_floors/floors JSON"};
            }

            FloorGroup fg;
            fg.group = getGroup(groupEntry["group"].get<std::string>());

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
    MappedFloor::getMaxGroupValue(const Group& group, const Manager& manager)
{
    std::optional<PropertyVariantType> max;
    bool checked = false;

    for (const auto& member : group.getMembers())
    {
        if (!manager.hasOwner(member, group.getInterface()))
        {
            continue;
        }

        try
        {
            auto value = Manager::getObjValueVariant(
                member, group.getInterface(), group.getProperty());

            // Only allow a group to have multiple members if it's numeric.
            // Unlike std::is_arithmetic, bools are not considered numeric here.
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

bool MappedFloor::groupOwnerDelayCheck(const Manager& manager)
{
    bool delaying = false;
    std::string missingOwners;

    auto findMissingOwners = [&manager](const Group& group,
                                        std::string& missingOwners) {
        std::for_each(group.getMembers().begin(), group.getMembers().end(),
                      [&manager, &missingOwners, &group](const auto& member) {
                          if (!manager.hasOwner(member, group.getInterface()))
                          {
                              missingOwners += member;
                          }
                      });
    };

    // Check for missing owners in the key and value groups
    findMissingOwners(*_keyGroup, missingOwners);

    for (const auto& floorTable : _fanFloors)
    {
        for (const auto& [group, floorGroups] : floorTable.floorGroups)
        {
            findMissingOwners(*group, missingOwners);
        }
    }

    if (!missingOwners.empty())
    {
        // If the same ones missing as last time, then delay is based
        // on if the timer is still running.
        if (missingOwners == _missingOwners)
        {
            delaying = _ownerTimer.isEnabled();
        }
        else
        {
            // Delay for 1 second if missing owner state is changed
            using namespace std::chrono_literals;
            _ownerTimer.restartOnce(1s);
            delaying = true;
        }
    }
    else
    {
        if (_ownerTimer.isEnabled())
        {
            _ownerTimer.setEnabled(false);
        }
    }

    // Save current missing owners to compare against next time
    _missingOwners = missingOwners;

    return delaying;
}

void MappedFloor::ownerTimerExpired()
{
    run(*_zone);
}

void MappedFloor::run(Zone& zone)
{
    _zone = &zone;
    std::optional<uint64_t> newFloor;
    bool missingGroupProperty = false;

    if (groupOwnerDelayCheck(*zone.getManager()))
    {
        return;
    }

    auto keyValue = getMaxGroupValue(*_keyGroup, *zone.getManager());
    if (!keyValue)
    {
        zone.setFloor(zone.getDefaultFloor());
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
        for (const auto& [group, floorGroups] : floorTable.floorGroups)
        {
            auto propertyValue = getMaxGroupValue(*group, *zone.getManager());
            if (!propertyValue)
            {
                // Couldn't successfully get a value.  Results in default floor.
                missingGroupProperty = true;
                break;
            }

            // Do either a <= or an == check depending on the data type to get
            // the floor value based on this group.
            std::optional<uint64_t> floor;
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

            // Keep track of the highest floor value found across all
            // entries/groups
            if (floor)
            {
                if ((newFloor && (floor > *newFloor)) || !newFloor)
                {
                    newFloor = floor;
                }
            }
            else
            {
                // No match found in this group's table.
                // Results in default floor.
                missingGroupProperty = true;
            }
        }

        // Valid key value for this entry, so done
        break;
    }

    if (newFloor && !missingGroupProperty)
    {
        zone.setFloor(*newFloor);
    }
    else
    {
        zone.setFloor(zone.getDefaultFloor());
    }
}

} // namespace phosphor::fan::control::json
