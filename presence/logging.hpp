#pragma once

#include "logger.hpp"

namespace phosphor::fan
{
/**
 * @brief Returns the singleton Logger class
 *
 * @return Logger& - The logger
 */
Logger& getLogger();
} // namespace phosphor::fan
