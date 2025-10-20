// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include "dbus_paths.hpp"
#include "fan.hpp"
#include "fan_error.hpp"
#include "multichassis_types.hpp"
#include "power_off_rule.hpp"
#include "power_state.hpp"
#include "tach_sensor.hpp"
#include "trust_manager.hpp"
#include "types.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/event.hpp>
#include <sdeventplus/source/signal.hpp>

#include <memory>
#include <optional>
#include <vector>

using json = nlohmann::json;

namespace phosphor::fan::monitor
{
// Mapping from service name to sensor
using SensorMapType =
    std::map<std::string, std::set<std::shared_ptr<TachSensor>>>;
class ZoneBase
{
  public:
    ZoneBase() = default;
    virtual ~ZoneBase() = default;
    ZoneBase(const ZoneBase&) = delete;
    ZoneBase& operator=(const ZoneBase&) = delete;
    ZoneBase(ZoneBase&&) = delete;
    ZoneBase& operator=(ZoneBase&&) = delete;

    /**
     * @brief Returns true if power is on
     */
    virtual bool isPowerOn() const = 0;

    /**
     * @brief Called from the fan when it changes either
     *        present or functional status to update the
     *        fan health map.
     *
     * @param[in] fan - The fan that changed
     * @param[in] skipRulesCheck - If the rules checks should be done now.
     */
    virtual void fanStatusChange(const Fan& fan,
                                 bool skipRulesCheck = false) = 0;

    /**
     * @brief Called when the timer that starts when a fan is missing
     *        has expired so an event log needs to be created.
     *
     * @param[in] fan - The missing fan.
     */
    virtual void fanMissingErrorTimerExpired(const Fan& fan) = 0;

    /**
     * @brief Called when a fan sensor's error timer expires, which
     *        happens when the sensor has been nonfunctional for a
     *        certain amount of time.  An event log will be created.
     *
     * @param[in] fan - The parent fan of the sensor
     * @param[in] sensor - The faulted sensor
     */
    virtual void sensorErrorTimerExpired(const Fan& fan,
                                         const TachSensor& sensor) = 0;

    /**
     * @brief Called by the power off actions to log an error when there is
     *        a power off due to fan problems.
     *
     * The error it logs is just the last fan error that occurred.
     */
    virtual void logShutdownError() = 0;
};
} // namespace phosphor::fan::monitor
