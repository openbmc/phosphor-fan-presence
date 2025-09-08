// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include "json_config.hpp"
#include "json_parser.hpp"
#include "multichassis_types.hpp"
#include "types.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>

namespace phosphor::fan::monitor::multi_chassis
{
/**
 * @brief Get the configured sensor definitions that make up a fan
 *
 * @param[in] sensors - JSON object containing a list of sensors
 *
 * @return List of sensor definition data that make up a fan being monitored
 */
const std::vector<SensorDefinition> getSensorDefs(const json& sensors);

/**
 * @brief Get the configured fan definitions to be monitored
 *
 * @param[in] obj - JSON object to parse from
 *
 * @return List of fan type definition data on the fans to be monitored
 */
const std::vector<FanTypeDefinition> getFanDefs(const json& fans);

/**
 * @brief Get the definitions for chassis configurations
 *
 * @param[in] chassis - JSON object containing a list of chassis configuration
 * objects
 *
 * @return List of chassis definition data that denotes how each chassis should
 * be configured
 */
const std::vector<ChassisDefinition> getChassisDefs(const json& chassis);

/**
 * @brief Get the zone definitions from the JSON and expand zone templates into
 * multiple zones based on chassis numbers
 *
 * @param[in] zones - JSON object containing a list of zone configuration
 * objects
 *
 * @param[in] chassisNums - List of chassis numbers for which to expand each
 * zone template
 *
 * @return - List of zone definition data that denotes how each zone should be
 * configured
 */
const std::vector<ZoneDefinition> getZoneDefs(
    const json& zones, const std::vector<int>& chassisNums);

/**
 * @brief Get the fan assignments for a zone config from a list of JSON objects
 *
 * @param[in] fanList - JSON object containing the fan assignments for a
 * particular zone config
 *
 * @param[in] chassisNum - Optional; Chassis number to substitute into
 * FanAssignment field values
 *
 * @return - List of fan assignment data that contains identifying information
 * for a fan
 */
const std::vector<FanAssignment> getFanAssigns(const json& fanList,
                                               std::optional<int> chassisNum);
} // namespace phosphor::fan::monitor::multi_chassis
