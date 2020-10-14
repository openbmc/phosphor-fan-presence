#pragma once

#include "logger.hpp"

namespace phosphor::fan::monitor
{
/**
 * @brief Returns the singleton Logger class
 *
 * @return Logger& - The logger
 */
Logger& getLogger();
} // namespace phosphor::fan::monitor
