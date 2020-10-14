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

#include "fan.hpp"
#include "power_off_rule.hpp"
#include "power_state.hpp"
#include "tach_sensor.hpp"
#include "trust_manager.hpp"
#include "types.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>

#include <memory>
#include <optional>
#include <vector>

namespace phosphor::fan::monitor
{

using json = nlohmann::json;

class System
{
  public:
    System() = delete;
    System(const System&) = delete;
    System(System&&) = delete;
    System& operator=(const System&) = delete;
    System& operator=(System&&) = delete;
    ~System() = default;

    /**
     * Constructor
     * Parses and populates the fan monitor trust groups and list of fans
     *
     * @param[in] mode - mode of fan monitor
     * @param[in] bus - sdbusplus bus object
     * @param[in] event - event loop reference
     */
    System(Mode mode, sdbusplus::bus::bus& bus,
           const sdeventplus::Event& event);

    /**
     * @brief Callback function to handle receiving a HUP signal to reload the
     * JSON configuration.
     */
    void sighupHandler(sdeventplus::source::Signal&,
                       const struct signalfd_siginfo*);

    /**
     * @brief Called from the fan when it changes either
     *        present or functional status to update the
     *        fan health map.
     *
     * @param[in] fan - The fan that changed
     */
    void fanStatusChange(const Fan& fan);

  private:
    /* The mode of fan monitor */
    Mode _mode;

    /* The sdbusplus bus object */
    sdbusplus::bus::bus& _bus;

    /* The event loop reference */
    const sdeventplus::Event& _event;

    /* Trust manager of trust groups */
    std::unique_ptr<phosphor::fan::trust::Manager> _trust;

    /* List of fan objects to monitor */
    std::vector<std::unique_ptr<Fan>> _fans;

    /**
     * @brief The latest health of all the fans
     */
    FanHealth _fanHealth;

    /**
     * @brief The object to watch the power state
     */
    std::unique_ptr<PowerState> _powerState;

    /**
     * @brief The power off rules, for shutting down the system
     *        due to fan failures.
     */
    std::vector<std::unique_ptr<PowerOffRule>> _powerOffRules;

    /**
     * @brief Retrieve the configured trust groups
     *
     * @param[in] jsonObj - JSON object to parse from
     *
     * @return List of functions applied on trust groups
     */
    const std::vector<CreateGroupFunction> getTrustGroups(const json& jsonObj);

    /**
     * @brief Set the trust manager's list of trust group functions
     *
     * @param[in] groupFuncs - list of trust group functions
     */
    void setTrustMgr(const std::vector<CreateGroupFunction>& groupFuncs);

    /**
     * @brief Retrieve the configured fan definitions
     *
     * @param[in] jsonObj - JSON object to parse from
     *
     * @return List of fan definition data on the fans configured
     */
    const std::vector<FanDefinition> getFanDefinitions(const json& jsonObj);

    /**
     * @brief Set the list of fans to be monitored
     *
     * @param[in] fanDefs - list of fan definitions to create fans monitored
     */
    void setFans(const std::vector<FanDefinition>& fanDefs);

    /**
     * @brief Updates the fan health map entry for the fan passed in
     *
     * @param[in] fan - The fan to update the health map with
     */
    void updateFanHealth(const Fan& fan);

    /**
     * @brief The function that runs when the power state changes
     *
     * @param[in] newPowerState - The new power state
     */
    void powerStateChanged(bool newPowerState);

    /**
     * @brief Reads the fault configuration from the JSON config
     *        file, such as the power off rule configuration.
     *
     * @param[in] jsonObj - JSON object to parse from
     */
    void setFaultConfig(const json& jsonObj);
};

} // namespace phosphor::fan::monitor
