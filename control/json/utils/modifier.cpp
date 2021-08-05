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

#include "modifier.hpp"

#include "json/manager.hpp"

#include <fmt/format.h>

#include <phosphor-logging/log.hpp>

using namespace phosphor::logging;

namespace phosphor::fan::control::json
{

const std::map<std::string, Modifier::Operator> Modifier::_operators{
    {"minus", Operator::minus}};

Modifier::Modifier(const json& jsonObj)
{
    setValue(jsonObj);
    setOperator(jsonObj);
}

void Modifier::setValue(const json& jsonObj)
{
    if (!jsonObj.contains("value"))
    {
        log<level::ERR>("modifier entry in JSON missing 'value'");
        throw std::invalid_argument("invalid modifier JSON");
    }

    const auto& object = jsonObj.at("value");
    if (auto boolPtr = object.get_ptr<const bool*>())
    {
        _value = *boolPtr;
    }
    else if (auto intPtr = object.get_ptr<const int64_t*>())
    {
        _value = *intPtr;
    }
    else if (auto doublePtr = object.get_ptr<const double*>())
    {
        _value = *doublePtr;
    }
    else if (auto stringPtr = object.get_ptr<const std::string*>())
    {
        _value = *stringPtr;
    }
    else
    {
        throw std::invalid_argument("invalid modifier JSON");
    }
}

void Modifier::setOperator(const json& jsonObj)
{
    if (!jsonObj.contains("operator"))
    {
        log<level::ERR>("modifier entry in JSON missing 'operator'");
        throw std::invalid_argument("invalid modifier JSON");
    }

    auto op = jsonObj["operator"].get<std::string>();

    auto e = _operators.find(op);
    if (e == _operators.end())
    {
        log<level::ERR>(
            fmt::format("operator {} in modifier JSON is invalid", op).c_str());
        throw std::invalid_argument("invalid modifier JSON");
    }

    _operator = e->second;
}

PropertyVariantType Modifier::doOp(const PropertyVariantType& value)
{
    // Just one op so far
    return minus(value);
}

/**
 * @brief Variant visitor to return a value of the template type specified.
 */
template <typename T>
struct ToTypeVisitor
{
    template <typename U>
    T operator()(const U& t) const
    {
        if constexpr (std::is_arithmetic_v<U> && std::is_arithmetic_v<T>)
        {
            return static_cast<T>(t);
        }
        throw std::invalid_argument(
            "Non arithmetic type used in ToTypeVisitor");
    }
};

PropertyVariantType Modifier::minus(const PropertyVariantType& val)
{
    PropertyVariantType result;

    if (std::holds_alternative<double>(val))
    {
        result =
            std::get<double>(val) - std::visit(ToTypeVisitor<double>(), _value);
    }
    else if (std::holds_alternative<int32_t>(val))
    {
        result = std::get<int32_t>(val) -
                 std::visit(ToTypeVisitor<int32_t>(), _value);
    }
    else if (std::holds_alternative<int64_t>(val))
    {
        result = std::get<int64_t>(val) -
                 std::visit(ToTypeVisitor<int64_t>(), _value);
    }
    else
    {
        log<level::ERR>(
            "Unsupported data type for modifier group member property value");
        throw std::invalid_argument{
            "Unsupported data type for modifier group member property value"};
    }

    return result;
}

} // namespace phosphor::fan::control::json
