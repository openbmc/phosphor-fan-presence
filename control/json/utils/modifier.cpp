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

/**
 * @brief Implements the minus operator to subtract two values.
 *
 * With strings values, A - B removes all occurrences of B in A.
 * Throws if the type is a bool.
 */
struct MinusOperator : public Modifier::BaseOperator
{
    PropertyVariantType operator()(double val) override
    {
        return val - std::visit(ToTypeVisitor<double>(), arg);
    }

    PropertyVariantType operator()(int32_t val) override
    {
        return val - std::visit(ToTypeVisitor<int32_t>(), arg);
    }

    PropertyVariantType operator()(int64_t val) override
    {
        return val - std::visit(ToTypeVisitor<int64_t>(), arg);
    }

    PropertyVariantType operator()(const std::string& val) override
    {
        // Remove all occurrences of arg from val.
        auto value = val;
        auto toRemove = std::get<std::string>(arg);
        size_t pos;
        while ((pos = value.find(toRemove)) != std::string::npos)
        {
            value.erase(pos, toRemove.size());
        }

        return value;
    }

    PropertyVariantType operator()(bool val) override
    {
        throw std::runtime_error{
            "Bool not allowed as a 'minus' modifier value"};
    }

    MinusOperator(PropertyVariantType& arg) : arg(arg)
    {}

    PropertyVariantType arg;
};

Modifier::Modifier(const json& jsonObj)
{
    setValue(jsonObj);
    setOperator(jsonObj);
}

void Modifier::setValue(const json& jsonObj)
{
    if (!jsonObj.contains("value"))
    {
        log<level::ERR>(
            fmt::format("Modifier entry in JSON missing 'value': {}",
                        jsonObj.dump())
                .c_str());
        throw std::invalid_argument("Invalid modifier JSON");
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
        log<level::ERR>(
            fmt::format(
                "Invalid JSON type for value property in modifer json: {}",
                jsonObj.dump())
                .c_str());
        throw std::invalid_argument("Invalid modifier JSON");
    }
}

void Modifier::setOperator(const json& jsonObj)
{
    if (!jsonObj.contains("operator"))
    {
        log<level::ERR>(
            fmt::format("Modifier entry in JSON missing 'operator': {}",
                        jsonObj.dump())
                .c_str());
        throw std::invalid_argument("Invalid modifier JSON");
    }

    auto op = jsonObj["operator"].get<std::string>();

    if (op == "minus")
    {
        _operator = std::make_unique<MinusOperator>(_value);
    }
    else
    {
        log<level::ERR>(fmt::format("Invalid operator in the modifier JSON: {}",
                                    jsonObj.dump())
                            .c_str());
        throw std::invalid_argument("Invalid operator in the modifier JSON");
    }
}

PropertyVariantType Modifier::doOp(const PropertyVariantType& val)
{
    return std::visit(*_operator, val);
}

} // namespace phosphor::fan::control::json
