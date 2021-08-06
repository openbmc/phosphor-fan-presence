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
#include "set_parameter_from_group.hpp"

#include "../manager.hpp"

namespace phosphor::fan::control::json
{

using json = nlohmann::json;
using namespace phosphor::logging;

SetParameterFromGroup::SetParameterFromGroup(const json& jsonObj,
                                             const std::vector<Group>& groups) :
    ActionBase(jsonObj, groups)
{
    setParameterName(jsonObj);
    setModifier(jsonObj);

    // Just support a single group until there is a need for more.
    if (groups.size() != 1)
    {
        throw ActionParseError{ActionBase::getName(),
                               "This action only supports 1 group"};
    }

    // Just support a single member group until there is a need for more.
    if (groups[0].getMembers().size() != 1)
    {
        throw ActionParseError{ActionBase::getName(),
                               "This action only supports 1 member groups"};
    }
}

void SetParameterFromGroup::run(Zone& zone)
{
    auto member = _groups[0].getMembers()[0];
    PropertyVariantType value;

    // Read the property value, possibly modify it, and then write
    // it to the Manager as a parameter.

    try
    {
        value = Manager::getObjValueVariant(member, _groups[0].getInterface(),
                                            _groups[0].getProperty());
    }
    catch (const std::invalid_argument& e)
    {
        return;
    }

    if (_modifier)
    {
        try
        {
            value = _modifier->doOp(value);
        }
        catch (const std::exception& e)
        {
            log<level::DEBUG>(
                fmt::format("{}: Could not perform modifier operation: {}",
                            ActionBase::getName(), e.what())
                    .c_str());
            return;
        }
    }

    zone.getManager()->setParameter(_name, value);
}

void SetParameterFromGroup::setParameterName(const json& jsonObj)
{
    if (!jsonObj.contains("parameter_name"))
    {
        throw ActionParseError{ActionBase::getName(),
                               "Missing required parameter_name value"};
    }

    _name = jsonObj["parameter_name"].get<std::string>();
}

void SetParameterFromGroup::setModifier(const json& jsonObj)
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
