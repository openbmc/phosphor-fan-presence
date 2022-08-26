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
#include "fan_error.hpp"
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

namespace phosphor::fan::monitor
{

using json = nlohmann::json;

// Mapping from service name to sensor
using SensorMapType =
    std::map<std::string, std::set<std::shared_ptr<TachSensor>>>;

class System
{
  public:
    System() = delete;
    ~System() = default;
    System(const System&) = delete;
    System(System&&) = delete;
    System& operator=(const System&) = delete;
    System& operator=(System&&) = delete;

    /**
     * Constructor
     *
     * @param[in] mode - mode of fan monitor
     * @param[in] bus - sdbusplus bus object
     * @param[in] event - event loop reference
     */
    System(Mode mode, sdbusplus::bus_t& bus, const sdeventplus::Event& event);

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
     * @param[in] skipRulesCheck - If the rules checks should be done now.
     */
    void fanStatusChange(const Fan& fan, bool skipRulesCheck = false);

    /**
     * @brief Called when a fan sensor's error timer expires, which
     *        happens when the sensor has been nonfunctional for a
     *        certain amount of time.  An event log will be created.
     *
     * @param[in] fan - The parent fan of the sensor
     * @param[in] sensor - The faulted sensor
     */
    void sensorErrorTimerExpired(const Fan& fan, const TachSensor& sensor);

    /**
     * @brief Called when the timer that starts when a fan is missing
     *        has expired so an event log needs to be created.
     *
     * @param[in] fan - The missing fan.
     */
    void fanMissingErrorTimerExpired(const Fan& fan);

    /**
     * @brief Called by the power off actions to log an error when there is
     *        a power off due to fan problems.
     *
     * The error it logs is just the last fan error that occurred.
     */
    void logShutdownError();

    /**
     * @brief Returns true if power is on
     */
    bool isPowerOn() const
    {
        return _powerState->isPowerOn();
    }

    /**
     * @brief tests the presence of Inventory and calls load() if present, else
     *  waits for Inventory asynchronously and has a callback to load() when
     * present
     */
    void start();

    /**
     * @brief Parses and populates the fan monitor trust groups and list of fans
     */
    void load();

    /**
     * @brief Callback function to handle receiving a USR1 signal to dump
     * debug data to a file.
     */
    void dumpDebugData(sdeventplus::source::Signal&,
                       const struct signalfd_siginfo*);

  private:
    /**
     * @brief Callback from D-Bus when Inventory service comes online
     *
     * @param[in] msg - Service details.
     */
    void inventoryOnlineCb(sdbusplus::message_t& msg);

    /**
     * @brief Create a BMC Dump
     */
    void createBmcDump() const;

    /* The mode of fan monitor */
    Mode _mode;

    /* The sdbusplus bus object */
    sdbusplus::bus_t& _bus;

    /* The event loop reference */
    const sdeventplus::Event& _event;

    /* Trust manager of trust groups */
    std::unique_ptr<phosphor::fan::trust::Manager> _trust;

    /* match object to detect Inventory service */
    std::unique_ptr<sdbusplus::bus::match_t> _inventoryMatch;

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
     * @brief The number of concurrently nonfunctional fan sensors
     *        there must be for an event log created due to a
     *        nonfunctional fan sensor to have an Error severity as
     *        opposed to an Informational one.
     */
    std::optional<size_t> _numNonfuncSensorsBeforeError;

    /**
     * @brief The most recently committed fan error.
     */
    std::unique_ptr<FanError> _lastError;

    /**
     * @brief The thermal alert D-Bus object
     */
    ThermalAlertObject _thermalAlert;

    /**
     * @brief The tach sensors D-Bus match objects
     */
    std::vector<std::unique_ptr<sdbusplus::bus::match_t>> _sensorMatch;

    /**
     * @brief true if config files have been loaded
     */
    bool _loaded = false;

    /**
     * @brief The name of the dump file
     */
    static const std::string dumpFile;

    /**
     * @brief Captures tach sensor data as JSON for use in
     *        fan fault and fan missing event logs.
     *
     * @return json - The JSON data
     */
    json captureSensorData();

    /**
     * @brief creates a subscription (service->sensor) to take sensors
     *        on/offline when D-Bus starts/stops updating values
     *
     */
    void subscribeSensorsToServices();

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
     * @brief callback when a tach sensor signal goes offline
     *
     * @param[in] msg - D-Bus message containing details (inc. service name)
     *
     * @param[in] sensorMap - map providing sensor access for each service
     */
    void tachSignalOffline(sdbusplus::message_t& msg,
                           const SensorMapType& sensorMap);

    /**
     * @brief The function that runs when the power state changes
     *
     * @param[in] powerStateOn - If power is now on or not
     */
    void powerStateChanged(bool powerStateOn);

    /**
     * @brief Reads the fault configuration from the JSON config
     *        file, such as the power off rule configuration.
     *
     * @param[in] jsonObj - JSON object to parse from
     */
    void setFaultConfig(const json& jsonObj);

    /**
     * @brief Log an error and shut down due to an offline fan controller
     */
    void handleOfflineFanController();
};

} // namespace phosphor::fan::monitor
