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
#pragma once

#include "config_base.hpp"

#include <string>
#include <vector>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

/**
 * @class Modifier
 *
 * This class provides a doOp() function to modify a PropertyVariantType value
 * based on a JSON config passed into its constructor.
 *
 * For example, with the JSON:
 * {
 *   "expression": "subtract",
 *   "value": 3
 * }
 *
 * When doOp() is called, it will subtract 3 from the value passed
 * into doOp() and return the result.
 *
 * The valid expressions are:
 *  - "subtract"
 */
class Modifier
{
  public:
    Modifier() = delete;
    ~Modifier() = default;
    Modifier(const Modifier&) = delete;
    Modifier& operator=(const Modifier&) = delete;
    Modifier(Modifier&&) = delete;
    Modifier& operator=(Modifier&&) = delete;

    /**
     * @brief Construction
     *
     * @param[in] jsonObj - The JSON config object
     */
    Modifier(const json& jsonObj);

    /**
     * @brief Performs the operation
     *
     * @param[in] value - The variant to do the operation on
     *
     * @return PropertyVariantType - The result
     */
    PropertyVariantType doOp(const PropertyVariantType& value);

  private:

    /**
     * @brief The available expressions that can be performed
     */
    enum class Expression
    {
        subtract
    };

    /**
     * @brief Parse and set the value
     *
     * @param[in] jsonObj - The JSON config object
     */
    void setValue(const json& jsonObj);

    /**
     * @brief Parse and set the expression
     *
     * @param[in] jsonObj - The JSON config object
     */
    void setExpression(const json& jsonObj);

    /**
     * @brief Subtracts _value from value
     *
     * @param[in] value - The value to subtract from
     */
    PropertyVariantType subtract(const PropertyVariantType& value);

    /**
     * @brief Map of valid expression names to their enums
     */
    static const std::map<std::string, Expression> _expressions;

    /** @brief The value used in the expression */
    PropertyVariantType _value;

    /** @brief The expression that will be performed */
    Expression _expression;
};

} // namespace phosphor::fan::control::json
