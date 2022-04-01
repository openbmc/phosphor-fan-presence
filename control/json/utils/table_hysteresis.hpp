/**
 * Copyright © 2022 IBM Corporation
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

#include "../config_base.hpp"

#include <chrono>
#include <optional>

namespace phosphor::fan::control::json
{

/**
 * @class TableHysteresis
 *
 * For use with the MappedFloor action when choosing the fan floor tables
 * based on the key value. It applies a hysteresis when the value is
 * decreasing, crossing to a new lower table, possibly preventing that
 * crossover if change in value isn't greater than the hysteresis
 * amount below the table cutoff.
 *
 * It also provides an optional timeout value so that the table change
 * won't be blocked indefinitely if the value settles close the value
 * in question.
 *
 * It should only be used when the key value is a double - i.e. a sensor
 * value.
 */
class TableHysteresis
{
  public:
    TableHysteresis() = delete;
    ~TableHysteresis() = default;
    TableHysteresis(const TableHysteresis&) = default;
    TableHysteresis& operator=(const TableHysteresis&) = default;
    TableHysteresis(TableHysteresis&&) = default;
    TableHysteresis& operator=(TableHysteresis&&) = default;

    /**
     * @brief Constructor
     *
     * @param[in] hysteresis - The hysteresis value to use
     * @param[in] timeoutSeconds - Timeout for blocking the
     *                             index change, if supplied.
     */
    TableHysteresis(double hysteresis,
                    const std::optional<size_t>& timeoutSeconds) :
        _hysteresis(hysteresis),
        _timeoutValue(timeoutSeconds)
    {}

    /**
     * @brief Chooses the table index to use after applying a
     *        hysteresis to decreasing values.
     *
     * Keeps track of the last index used so it knows when it would
     * be changing.
     *
     * @param[in] currentValue - The current key value
     * @param[in] indexAndCutoff - The current table index and
     *                             cutoff value for the table
     *
     * @return size_t - The index to use for the table
     */
    size_t chooseIndex(
        const PropertyVariantType& currentValue,
        const std::tuple<size_t, PropertyVariantType>& indexAndCutoff)
    {
        const auto& [index, cutoffVariant] = indexAndCutoff;

        if (!std::holds_alternative<double>(currentValue) ||
            !std::holds_alternative<double>(cutoffVariant))
        {
            throw std::runtime_error(
                "TableHysteresis configured but values not doubles");
        }

        const double& cutoff = std::get<double>(cutoffVariant);
        const double& value = std::get<double>(currentValue);

        // First time through just save value and return index passed in
        if (!_previousIndex)
        {
            _previousIndex = index;
            return index;
        }

        // Default to choosing the index passed in
        size_t indexToUse = index;

        // If we are going to a lower table index based on a lower value,
        // and the value is less than the hysteresis amount from the
        // table cutoff, then don't switch tables so use the previous
        // table index.
        if ((index < *_previousIndex) && (value < cutoff) &&
            ((cutoff - value) <= _hysteresis))
        {
            auto now = std::chrono::steady_clock::now();

            // If the timeout value is configured, then blocking the
            // table change will stop after that time has passed so
            // the table change can proceed.
            // If no timeout configured, the table change can be
            // blocked as long as within the hysteresis amount.
            auto timedOut = false;

            if (_timeoutValue && _blockStartTime &&
                (now >= (*_blockStartTime + *_timeoutValue)))
            {
                timedOut = true;
            }

            if (_blockStartTime && timedOut)
            {
                // Timed out, allow index change and get ready
                // for next time.
                _previousIndex = index;
                _blockStartTime = std::nullopt;
            }
            else
            {
                // Floor table change is blocked, use previous index

                // Capture current time to use in the timeout if configured
                if (_timeoutValue && !_blockStartTime)
                {
                    _blockStartTime = now;
                }

                indexToUse = *_previousIndex;
            }
        }
        else
        {
            // Normal case, allow index change and get ready
            // for next time.
            _previousIndex = index;
            _blockStartTime = std::nullopt;
        }

        return indexToUse;
    }

  private:
    double _hysteresis;
    std::optional<std::chrono::seconds> _timeoutValue;
    std::optional<std::chrono::time_point<std::chrono::steady_clock>>
        _blockStartTime;
    std::optional<size_t> _previousIndex;
};

} // namespace phosphor::fan::control::json
