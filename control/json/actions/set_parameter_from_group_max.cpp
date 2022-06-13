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
#include "set_parameter_from_group_max.hpp"

#include "../manager.hpp"

#include <fmt/format.h>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using namespace phosphor::logging;

SetParameterFromGroupMax::SetParameterFromGroupMax(
    const json& jsonObj, const std::vector<Group>& groups) :
    ActionBase(jsonObj, groups)
{
    setParameterName(jsonObj);
    setModifier(jsonObj);
}

void SetParameterFromGroupMax::run(Zone& /*zone*/)
{
    std::optional<PropertyVariantType> max;

    // Find the maximum value of all group member properties, possibly modify
    // it, and then write it to the Manager as a parameter.

    for (const auto& group : _groups)
    {
        const auto& members = group.getMembers();
        for (const auto& member : members)
        {
            PropertyVariantType value;
            try
            {
                value = Manager::getObjValueVariant(
                    member, group.getInterface(), group.getProperty());
            }
            catch (const std::out_of_range&)
            {
                continue;
            }

            // Only allow a group to have multiple members if it's
            // numeric. Unlike with std::is_arithmetic, bools are not
            // considered numeric here.
            if (members.size() > 1)
            {
                bool invalid = false;
                std::visit(
                    [&group, &invalid, this](auto&& val) {
                        using V = std::decay_t<decltype(val)>;
                        if constexpr (!std::is_same_v<double, V> &&
                                      !std::is_same_v<int32_t, V> &&
                                      !std::is_same_v<int64_t, V>)
                        {
                            log<level::ERR>(fmt::format("{}: Group {} has more "
                                                        "than one member but "
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
                    continue;
                }
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

    if (_modifier && max)
    {
        try
        {
            *max = _modifier->doOp(*max);
        }
        catch (const std::exception& e)
        {
            log<level::ERR>(
                fmt::format("{}: Could not perform modifier operation: {}",
                            ActionBase::getName(), e.what())
                    .c_str());
            return;
        }
    }

    Manager::setParameter(_name, max);
}

void SetParameterFromGroupMax::setParameterName(const json& jsonObj)
{
    if (!jsonObj.contains("parameter_name"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required parameter_name value"};
    }

    _name = jsonObj["parameter_name"].get<std::string>();
}

void SetParameterFromGroupMax::setModifier(const json& jsonObj)
{
    if (jsonObj.contains("modifier"))
    {
        try
        {
            _modifier = std::make_unique<Modifier>(jsonObj.at("modifier"));
        }
        catch (const std::invalid_argument& e)
        {
            throw ActionParseError{ActionBase::getName(), e.what()};
        }
    }
}

}; // namespace phosphor::fan::control::json
