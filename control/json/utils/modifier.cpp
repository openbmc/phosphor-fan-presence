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
    MinusOperator(const json& valueObj)
    {
        if (auto boolPtr = valueObj.get_ptr<const bool*>())
        {
            arg = *boolPtr;
        }
        else if (auto intPtr = valueObj.get_ptr<const int64_t*>())
        {
            arg = *intPtr;
        }
        else if (auto doublePtr = valueObj.get_ptr<const double*>())
        {
            arg = *doublePtr;
        }
        else if (auto stringPtr = valueObj.get_ptr<const std::string*>())
        {
            arg = *stringPtr;
        }
        else
        {
            log<level::ERR>(
                fmt::format(
                    "Invalid JSON type for value property in modifier json: {}",
                    valueObj.dump())
                    .c_str());
            throw std::invalid_argument("Invalid modifier JSON");
        }
    }

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

    PropertyVariantType arg;
};

/**
 * @brief Implements an operator to return values from the JSON
 *        config when a key value is within a certain range.
 *
 *       "modifier": {
 *        "operator": "less_than_ranges",
 *        "value": [
 *          {
 *            "arg_value": 30, // arg val must be less than this
 *            "parameter_value": 300
 *          },
 *          {
 *            "arg_value": 40,
 *            "parameter_value": 400
 *          },
 *        ]
 *        }
 * If a arg_value is out of the range, it just returns the last
 * parameter value.
 */
struct LessThanRangesOperator : public Modifier::BaseOperator
{
    LessThanRangesOperator(const json& valueArray)
    {
        if (!valueArray.is_array())
        {
            log<level::ERR>(
                fmt::format("Invalid JSON data for less_than_ranges config: {}",
                            valueArray.dump())
                    .c_str());
            throw std::invalid_argument("Invalid modifier JSON");
        }

        for (const auto& valueEntry : valueArray)
        {
            if (!valueEntry.contains("arg_value") ||
                !valueEntry.contains("parameter_value"))
            {
                log<level::ERR>(
                    fmt::format("Missing arg_value or parameter_value keys "
                                "in less_than_ranges config: {}",
                                valueArray.dump())
                        .c_str());
                throw std::invalid_argument("Invalid modifier JSON");
            }

            const auto& argValObj = valueEntry.at("arg_value");
            const auto& paramValObj = valueEntry.at("parameter_value");
            PropertyVariantType argVal;
            PropertyVariantType paramVal;

            if (auto intPtr = argValObj.get_ptr<const int64_t*>())
            {
                argVal = *intPtr;
            }
            else if (auto doublePtr = argValObj.get_ptr<const double*>())
            {
                argVal = *doublePtr;
            }
            else if (auto stringPtr = argValObj.get_ptr<const std::string*>())
            {
                argVal = *stringPtr;
            }
            else
            {
                log<level::ERR>(
                    fmt::format(
                        "Invalid data type in arg_value key in modifier JSON "
                        "config: {}",
                        valueArray.dump())
                        .c_str());
                throw std::invalid_argument("Invalid modifier JSON");
            }

            if (auto boolPtr = paramValObj.get_ptr<const bool*>())
            {
                paramVal = *boolPtr;
            }
            else if (auto intPtr = paramValObj.get_ptr<const int64_t*>())
            {
                paramVal = *intPtr;
            }
            else if (auto doublePtr = paramValObj.get_ptr<const double*>())
            {
                paramVal = *doublePtr;
            }
            else if (auto stringPtr = paramValObj.get_ptr<const std::string*>())
            {
                paramVal = *stringPtr;
            }
            else
            {
                log<level::ERR>(
                    fmt::format(
                        "Invalid data type in param_value key in modifier JSON "
                        "config: {}",
                        valueArray.dump())
                        .c_str());
                throw std::invalid_argument("Invalid modifier JSON");
            }

            rangeValues.emplace_back(argVal, paramVal);
        }

        if (rangeValues.empty())
        {
            log<level::ERR>(fmt::format("No valid range values found in "
                                        "modifier json: {}",
                                        valueArray.dump())
                                .c_str());
            throw std::invalid_argument("Invalid modifier JSON");
        }
    }

    PropertyVariantType operator()(double val) override
    {
        for (const auto& rangeValue : rangeValues)
        {
            if (val < std::visit(ToTypeVisitor<double>(), rangeValue.first))
            {
                return rangeValue.second;
            }
        }
        log<level::DEBUG>(
            fmt::format("Value {} out of range in 'less_than_ranges' modifier",
                        val)
                .c_str());

        // Return the value from the last entry
        return rangeValues.back().second;
    }

    PropertyVariantType operator()(int32_t val) override
    {
        for (const auto& rangeValue : rangeValues)
        {
            if (val < std::visit(ToTypeVisitor<int32_t>(), rangeValue.first))
            {
                return rangeValue.second;
            }
        }

        log<level::DEBUG>(
            fmt::format("Value {} out of range in 'less_than_ranges' modifier",
                        val)
                .c_str());
        return rangeValues.back().second;
    }

    PropertyVariantType operator()(int64_t val) override
    {
        for (const auto& rangeValue : rangeValues)
        {
            if (val < std::visit(ToTypeVisitor<int64_t>(), rangeValue.first))
            {
                return rangeValue.second;
            }
        }
        log<level::DEBUG>(
            fmt::format("Value {} out of range in 'less_than_ranges' modifier",
                        val)
                .c_str());
        return rangeValues.back().second;
    }

    PropertyVariantType operator()(const std::string& val) override
    {
        for (const auto& rangeValue : rangeValues)
        {
            if (val <
                std::visit(ToTypeVisitor<std::string>(), rangeValue.first))
            {
                return rangeValue.second;
            }
        }
        log<level::DEBUG>(
            fmt::format("Value {} out of range in 'less_than_ranges' modifier",
                        val)
                .c_str());
        return rangeValues.back().second;
    }

    PropertyVariantType operator()(bool val) override
    {
        throw std::runtime_error{
            "Bool not allowed as a 'less_than_ranges' modifier value"};
    }

    std::vector<std::pair<PropertyVariantType, PropertyVariantType>>
        rangeValues;
};

Modifier::Modifier(const json& jsonObj)
{
    setOperator(jsonObj);
}

void Modifier::setOperator(const json& jsonObj)
{
    if (!jsonObj.contains("operator") || !jsonObj.contains("value"))
    {
        log<level::ERR>(
            fmt::format(
                "Modifier entry in JSON missing 'operator' or 'value': {}",
                jsonObj.dump())
                .c_str());
        throw std::invalid_argument("Invalid modifier JSON");
    }

    auto op = jsonObj["operator"].get<std::string>();

    if (op == "minus")
    {
        _operator = std::make_unique<MinusOperator>(jsonObj["value"]);
    }
    else if (op == "less_than_ranges")
    {
        _operator = std::make_unique<LessThanRangesOperator>(jsonObj["value"]);
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
