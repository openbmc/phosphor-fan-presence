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
 * @brief The enum to represent the type of Alarm
 */
enum class AlarmType
{
    hardShutdown,
    softShutdown,
    critical,
    warning
};

/**
 * @brief The enum to represent a low or high alarm
 */
enum class AlarmDirection
{
    low,
    high
};

using AlarmKey = std::tuple<std::string, AlarmType, AlarmDirection>;

} // namespace sensor::monitor
