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

#include "json/config_base.hpp"
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
 * @brief Return a default value to use when the argument passed
 *        to LessThanOperator is out of range.
 */
PropertyVariantType
    getDefaultValue(const PropertyVariantType& val,
                    const std::optional<PropertyVariantType>& defaultValue)
{
    // When a default value is given, return that
    if (defaultValue)
    {
        return *defaultValue;
    }

    if (std::holds_alternative<bool>(val))
    {
        return false;
    }
    else if (std::holds_alternative<std::string>(val))
    {
        return std::string{};
    }
    else if (std::holds_alternative<double>(val))
    {
        return std::numeric_limits<double>::quiet_NaN();
    }
    else if (std::holds_alternative<int32_t>(val))
    {
        return std::numeric_limits<int32_t>::quiet_NaN();
    }
    else if (std::holds_alternative<int64_t>(val))
    {
        return std::numeric_limits<int64_t>::quiet_NaN();
    }

    throw std::runtime_error(
        "Invalid variant type when determining default value");
}

/**
 * @brief Implements the minus operator to subtract two values.
 *
 * With strings values, A - B removes all occurrences of B in A.
 * Throws if the type is a bool.
 */
struct MinusOperator : public Modifier::BaseOperator
{
    explicit MinusOperator(const json& jsonObj) :
        arg(ConfigBase::getJsonValue(jsonObj["value"]))
    {}

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

    PropertyVariantType operator()(bool) override
    {
        throw std::runtime_error{
            "Bool not allowed as a 'minus' modifier value"};
    }

    PropertyVariantType arg;
};

/**
 * @brief Implements an operator to return a value specified in the JSON that is
 * chosen based on if the value passed into the operator is less than the lowest
 * arg_value it is true for or the given `default_value` if not found to be less
 * than any entries.
 *
 * "modifier": {
 *  "operator": "less_than",
 *  "default_value": 1000, // OPTIONAL
 *  "value": [
 *    {
 *      "arg_value": 30, // if value is less than 30
 *      "parameter_value": 300  // then return 300
 *    },
 *    {
 *      "arg_value": 40, // else if value is less than 40
 *      "parameter_value": 400 // then return 400
 *    },
 *   ]
 *  }
 *
 * If the value passed in is higher than the highest arg_value, it returns a
 * default value this is the `default_value` given or based on the data type of
 * parameter_value.
 */
struct LessThanOperator : public Modifier::BaseOperator
{
    explicit LessThanOperator(const json& jsonObj)
    {
        const auto& valueArray = jsonObj["value"];
        if (!valueArray.is_array())
        {
            log<level::ERR>(
                fmt::format("Invalid JSON data for less_than config: {}",
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
                                "in less_than config: {}",
                                valueArray.dump())
                        .c_str());
                throw std::invalid_argument("Invalid modifier JSON");
            }

            auto argVal = ConfigBase::getJsonValue(valueEntry.at("arg_value"));

            if (std::holds_alternative<bool>(argVal))
            {
                log<level::ERR>(
                    fmt::format(
                        "Invalid data type in arg_value key in modifier JSON "
                        "config: {}",
                        valueArray.dump())
                        .c_str());
                throw std::invalid_argument("Invalid modifier JSON");
            }

            auto paramVal =
                ConfigBase::getJsonValue(valueEntry.at("parameter_value"));

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

        if (jsonObj.contains("default_value"))
        {
            defaultValue = ConfigBase::getJsonValue(jsonObj["default_value"]);
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
        // Return a default value based on last entry type
        return getDefaultValue(rangeValues.back().second, defaultValue);
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
        return getDefaultValue(rangeValues.back().second, defaultValue);
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
        return getDefaultValue(rangeValues.back().second, defaultValue);
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
        return getDefaultValue(rangeValues.back().second, defaultValue);
    }

    PropertyVariantType operator()(bool) override
    {
        throw std::runtime_error{
            "Bool not allowed as a 'less_than' modifier value"};
    }

    std::vector<std::pair<PropertyVariantType, PropertyVariantType>>
        rangeValues;
    std::optional<PropertyVariantType> defaultValue;
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
        _operator = std::make_unique<MinusOperator>(jsonObj);
    }
    else if (op == "less_than")
    {
        _operator = std::make_unique<LessThanOperator>(jsonObj);
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
