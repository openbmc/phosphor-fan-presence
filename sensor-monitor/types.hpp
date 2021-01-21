#pragma once

#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <memory>
#include <string>
#include <tuple>

namespace sensor::monitor
{

/**
 * @brief The enum to represent a hard or soft shutdown
 */
enum class ShutdownType
{
    hard,
    soft
};

/**
 * @brief The enum to represent a low or high alarm
 */
enum class AlarmType
{
    low,
    high
};

using AlarmKey = std::tuple<std::string, ShutdownType, AlarmType>;
} // namespace sensor::monitor
