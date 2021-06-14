#pragma once
#include <nlohmann/json.hpp>

namespace phosphor::fan::monitor
{

/**
 * @brief Collects hwmon data for event log FFDC
 *
 * Makes a list of the loaded hwmon driver names, and
 * pulls interesting lines from dmesg.
 *
 * @return json - The FFDC data
 */
nlohmann::json collectHwmonFFDC();

} // namespace phosphor::fan::monitor
