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
            /*The speed derived from mapping*/
            uint64_t groupSpeed = 0;

            // Value is decreasing from previous  && greater than positive
            // hysteresis
            if ((groupValue < _prevGroupValue) &&
                (_prevGroupValue - groupValue > _posHysteresis))
            {
                for (auto it = _valueToSpeedMap.rbegin();
                     it != _valueToSpeedMap.rend(); ++it)
                {
                    // Value is at/above last map key, set speed to the last map
                    // key's value
                    if (it == _valueToSpeedMap.rbegin() &&
                        groupValue >= it->first)
                    {
                        groupSpeed = it->second;
                        break;
                    }
                    // Value is at/below first map key, set speed to the first
                    // map key's value
                    else if (std::next(it, 1) == _valueToSpeedMap.rend() &&
                             groupValue <= it->first)
                    {
                        groupSpeed = it->second;
                        break;
                    }
                    // Value decreased & transitioned across a map key, update
                    // speed to this map key's value when new value is at or
                    // below map's key and the key is at/below the previous
                    // value
                    if (groupValue <= it->first && it->first <= _prevGroupValue)
                    {
                        groupSpeed = it->second;
                    }
                }
                _prevGroupValue = groupValue;
                _speedFromGroupsMap[_groupIndex] = groupSpeed;

                // Get the maximum speed derived from all groups, and set target
                // for the Zone
                auto maxSpeedFromGroupsIter = std::max_element(
                    _speedFromGroupsMap.begin(), _speedFromGroupsMap.end(),
                    [](const auto& x, const auto& y) {
                        return x.second < y.second;
                    });

                zone.setTarget(maxSpeedFromGroupsIter->second);
            }
            // Value is increasing from previous && greater than negative
            // hysteresis
            else
            {
                if (groupValue - _prevGroupValue > _negHysteresis)
                {
                    for (auto it = _valueToSpeedMap.begin();
                         it != _valueToSpeedMap.end(); ++it)
                    {
                        // Value is at/below the first map key, set speed to the
                        // first map key's value
                        if (it == _valueToSpeedMap.begin() &&
                            groupValue <= it->first)
                        {
                            groupSpeed = it->second;
                            break;
                        }
                        // Value is at/above last map key, set speed to the last
                        // map key's value
                        else if (std::next(it, 1) == _valueToSpeedMap.end() &&
                                 groupValue >= it->first)
                        {
                            groupSpeed = it->second;
                            break;
                        }
                        // Value increased & transitioned across a map key,
                        // update speed to the next map key's value when new
                        // value is above map's key and the key is at/above the
                        // previous value
                        if (groupValue > it->first &&
                            it->first >= _prevGroupValue)
                        {
                            groupSpeed = std::next(it, 1)->second;
                        }
                        // Value increased & transitioned across a map key,
                        // update speed to the map key's value when new value is
                        // at the map's key and the key is above the previous
                        // value
                        else if (groupValue == it->first &&
                                 it->first > _prevGroupValue)
                        {
                            groupSpeed = it->second;
                        }
                    }
                }
                _prevGroupValue = groupValue;
                _speedFromGroupsMap[_groupIndex] = groupSpeed;

                // Get the maximum speed derived from all groups, and set target
                // for the Zone
                auto maxSpeedFromGroupsIter = std::max_element(
                    _speedFromGroupsMap.begin(), _speedFromGroupsMap.end(),
                    [](const auto& x, const auto& y) {
                        return x.second < y.second;
                    });

                zone.setTarget(maxSpeedFromGroupsIter->second);
            }
        }
    }
    else
    {
        std::cerr << "Failed to process groups for " << ActionBase::getName()
                  << ": Further processing will be skipped \n";
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

}; // namespace phosphor::fan::control::json
