// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include "fan.hpp"
#include "fan_error.hpp"
#include "hwmon_ffdc.hpp"
#include "json_parser.hpp"
#include "logging.hpp"
#include "multichassis_json_parser.hpp"
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

namespace phosphor::fan::monitor::multi_chassis
{
class Zone : public phosphor::fan::monitor::ZoneBase
{
  public:
    Zone() = delete;
    Zone(const Zone&) = delete;
    Zone(Zone&&) = delete;
    Zone& operator=(const Zone&) = delete;
    Zone& operator=(Zone&&) = delete;
    ~Zone() = default;

    /**
     * Constructor
     *
     * @param[in] zoneConfig - ZoneDefinition object representing a specific
     * zone
     * @param[in] fanDefs - Vector of FanTypeDefinition objects representing the
     * different types of fans in the system
     * @param[in] bus - sdbusplus object
     * @param[in] mode - The mode of the fan monitor
     * @param[in] event - Event loop reference
     * @param[in] thermalAlert - Reference to ThermalAlertObject
     *
     */
    Zone(const ZoneDefinition& zoneConfig,
         const std::vector<FanTypeDefinition>& fanDefs, sdbusplus::bus_t& bus,
         Mode mode, const sdeventplus::Event& event,
         ThermalAlertObject& thermalAlert, PowerState& powerState);

    /**
     * @brief Constructs a FanDefinition object based on a provided
     * FanTypeDefinition and FanAssignment
     *
     * @param[in] fanType - The fan type for the corresposing FanDefinition
     * @param[in] fanAssign - FanAssignment object that ties the fan to a
     * specific zone
     *
     * @return FanDefinition - Full representation of the fan that is used to
     * initialize the corresponding Fan object
     */
    FanDefinition getFullDefFromType(const FanTypeDefinition& fanType,
                                     const FanAssignment& fanAssign);

    /**
     * @brief Creates fan objects and assigns them to zones based on the zone
     * configuration and the fan type definition (found using the fan type from
     * the zone config)
     *
     * @param[in] zoneConfig - Zone configuration object
     * @param[in] fanDefs - Vector of FanTypeDefinition objects
     */
    void setFans(const ZoneDefinition& zoneConfig,
                 const std::vector<FanTypeDefinition>& fanDefs);

    /**
     * @brief Initialize the zone by setting relevant fields
     *
     * @param[in] zoneConfig - Zone configuration object
     * @param[in] fanDefs - Vector of FanTypeDefinition objects
     */
    void init(const ZoneDefinition& zoneConfig,
              const std::vector<FanTypeDefinition>& fanDefs);

    /**
     * @brief Returns true if power is on
     */
    bool isPowerOn() const override
    {
        return _powerState.isPowerOn();
    }

    /**
     * @brief Called from the fan when it changes either
     *        present or functional status to update the
     *        fan health map.
     *
     * @param[in] fan - The fan that changed
     * @param[in] skipRulesCheck - If the rules checks should be done now.
     */
    void fanStatusChange(const Fan& fan, bool skipRulesCheck = false) override;

    /**
     * @brief Called when the timer that starts when a fan is missing
     *        has expired so an event log needs to be created.
     *
     * @param[in] fan - The missing fan.
     */
    void fanMissingErrorTimerExpired(const Fan& fan) override;

    /**
     * @brief Called when a fan sensor's error timer expires, which
     *        happens when the sensor has been nonfunctional for a
     *        certain amount of time.  An event log will be created.
     *
     * @param[in] fan - The parent fan of the sensor
     * @param[in] sensor - The faulted sensor
     */
    void sensorErrorTimerExpired(const Fan& fan,
                                 const TachSensor& sensor) override;

    /**
     * @brief Called by the power off actions to log an error when there is
     *        a power off due to fan problems.
     *
     * The error it logs is just the last fan error that occurred.
     */
    void logShutdownError() override;

    /**
     * @brief Getter for fans vector. Returns a vector of unique pointers to
     * Fan objects.
     */
    const std::vector<std::unique_ptr<Fan>>& getFans() const
    {
        return _fans;
    }

  private:
    /* Zone configuration object */
    ZoneDefinition _zoneConfig;

    /* Vector of fan types in the system */
    std::vector<FanTypeDefinition> _fanDefs;

    /* The Zone name */
    std::string _name;

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
    ThermalAlertObject& _thermalAlert;

    /**
     * @brief PowerState object reference
     */
    PowerState& _powerState;

    /**
     * @brief The tach sensors D-Bus match objects
     */
    std::vector<std::unique_ptr<sdbusplus::bus::match_t>> _sensorMatch;

    /**
     * @brief true if config files have been loaded
     */
    bool _loaded = false;

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

    /**
     * @brief Create a BMC Dump
     */
    void createBmcDump() const;

    /**
     * @brief Callback from D-Bus when Inventory service comes online
     *
     * @param[in] msg - Service details.
     */
    void inventoryOnlineCb(sdbusplus::message_t& msg);
};
} // namespace phosphor::fan::monitor::multi_chassis
