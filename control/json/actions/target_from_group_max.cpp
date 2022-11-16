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
#include "target_from_group_max.hpp"

#include "../manager.hpp"

#include <fmt/format.h>

#include <iostream>

namespace phosphor::fan::control::json
{

std::map<size_t, uint64_t> TargetFromGroupMax::_speedFromGroupsMap;
size_t TargetFromGroupMax::_groupIndexCounter = 0;

using json = nlohmann::json;
using namespace phosphor::logging;

TargetFromGroupMax::TargetFromGroupMax(const json& jsonObj,
                                       const std::vector<Group>& groups) :
    ActionBase(jsonObj, groups)
{
    setHysteresis(jsonObj);
    setMap(jsonObj);
    setIndex();
}

void TargetFromGroupMax::run(Zone& zone)
{
    // Holds the max property value of groups
    auto maxGroup = processGroups();

    // Group with non-numeric property value will be skipped from processing
    if (maxGroup)
    {
        /*The maximum property value from the group*/
        uint64_t groupValue =
            static_cast<uint64_t>(std::get<double>(maxGroup.value()));

        // Only check if previous and new values differ
        if (groupValue != _prevGroupValue)
        {
            // Value is decreasing and the change is greater than the
            // positive hysteresis; or value is increasing and the change
            // is greater than the positive hysteresis
            if (((groupValue < _prevGroupValue) &&
                 (_prevGroupValue - groupValue > _posHysteresis)) ||
                ((groupValue > _prevGroupValue) &&
                 (groupValue - _prevGroupValue > _negHysteresis)))
            {
                /*The speed derived from mapping*/
                uint64_t groupSpeed = _speedFromGroupsMap[_groupIndex];

                // Looping through the mapping table
                for (auto it = _valueToSpeedMap.begin();
                     it != _valueToSpeedMap.end(); ++it)
                {
                    // Value is at/below the first map key, or at/above the
                    // last map key, set speed to the first or the last map
                    // key's value respectively
                    if (((it == _valueToSpeedMap.begin()) &&
                         (groupValue <= it->first)) ||
                        ((std::next(it, 1) == _valueToSpeedMap.end()) &&
                         (groupValue >= it->first)))
                    {
                        groupSpeed = it->second;
                        break;
                    }
                    // Value transitioned across a map key, update the speed
                    // to this map key's value when the new group value is at
                    // or above the map's key and below the next key
                    if (groupValue >= it->first &&
                        groupValue < std::next(it, 1)->first)
                    {
                        groupSpeed = it->second;
                        break;
                    }
                }
                _prevGroupValue = groupValue;
                _speedFromGroupsMap[_groupIndex] = groupSpeed;
            }
        }
        // Get the maximum speed derived from all groups, and set target
        // for the Zone
        auto maxSpeedFromGroupsIter = std::max_element(
            _speedFromGroupsMap.begin(), _speedFromGroupsMap.end(),
            [](const auto& x, const auto& y) { return x.second < y.second; });

        zone.setTarget(maxSpeedFromGroupsIter->second);
    }
    else
    {
        // std::cerr << "Failed to process groups for " << ActionBase::getName()
        //           << ": Further processing will be skipped \n";
    }
}

void TargetFromGroupMax::setHysteresis(const json& jsonObj)
{
    if (!jsonObj.contains("neg_hysteresis") ||
        !jsonObj.contains("pos_hysteresis"))
    {
        throw ActionParseError{
            ActionBase::getName(),
            "Missing required neg_hysteresis or pos_hysteresis value"};
    }
    _negHysteresis = jsonObj["neg_hysteresis"].get<uint64_t>();
    _posHysteresis = jsonObj["pos_hysteresis"].get<uint64_t>();
}

void TargetFromGroupMax::setIndex()
{
    _groupIndex = _groupIndexCounter;
    // Initialize the map of each group and their max values
    _speedFromGroupsMap[_groupIndex] = 0;

    // Increase the index counter by one to specify the next group key
    _groupIndexCounter += 1;
}

void TargetFromGroupMax::setMap(const json& jsonObj)
{
    if (jsonObj.contains("map"))
    {
        for (const auto& map : jsonObj.at("map"))
        {

            if (!map.contains("value") || !map.contains("target"))
            {
                throw ActionParseError{ActionBase::getName(),
                                       "Missing value or target in map"};
            }
            else
            {
                uint64_t val = map["value"].get<uint64_t>();
                uint64_t target = map["target"].get<uint64_t>();
                _valueToSpeedMap.insert(
                    std::pair<uint64_t, uint64_t>(val, target));
            }
        }
    }

    else
    {
        throw ActionParseError{ActionBase::getName(), "Missing required map"};
    }
}

std::optional<PropertyVariantType> TargetFromGroupMax::processGroups()
{
    // Holds the max property value of groups
    std::optional<PropertyVariantType> max;

    for (const auto& group : _groups)
    {
        const auto& members = group.getMembers();
        for (const auto& member : members)
        {
            PropertyVariantType value;
            bool invalid = false;
            try
            {
                value = Manager::getObjValueVariant(
                    member, group.getInterface(), group.getProperty());
            }
            catch (const std::out_of_range&)
            {
                continue;
            }

            // Only allow a group members to be
            // numeric. Unlike with std::is_arithmetic, bools are not
            // considered numeric here.
            std::visit(
                [&group, &invalid, this](auto&& val) {
                    using V = std::decay_t<decltype(val)>;
                    if constexpr (!std::is_same_v<double, V> &&
                                  !std::is_same_v<int32_t, V> &&
                                  !std::is_same_v<int64_t, V>)
                    {
                        log<level::ERR>(fmt::format("{}: Group {}'s member "
                                                    "isn't numeric",
                                                    ActionBase::getName(),
                                                    group.getName())
                                            .c_str());
                        invalid = true;
                    }
                },
                value);
            if (invalid)
            {
                break;
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
    }
    return max;
}

} // namespace phosphor::fan::control::json
