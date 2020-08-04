/**
 * Copyright © 2020 IBM Corporation
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

#include "types.hpp"

#include <sdbusplus/bus.hpp>

namespace phosphor::fan::control
{

constexpr auto confAppName = "control";

/**
 * @brief Get the configuration definitions for zone groups
 *
 * @param[in] bus - The dbus bus object
 *
 * @return List of zone group objects
 *     Generated list of zone groups including their control configurations
 */
const std::vector<ZoneGroup> getZoneGroups(sdbusplus::bus::bus& bus);

/**
 * @brief Get the delay(in seconds) to allow the fans to ramp up to the defined
 * power on speed
 *
 * @param[in] bus - The dbus bus object
 *
 * @return Time to delay in seconds
 *     Amount of time to delay in seconds after a power on
 */
const unsigned int getPowerOnDelay(sdbusplus::bus::bus& bus);

} // namespace phosphor::fan::control
