#pragma once
#include "config.h"

#include "dbus_alarm_monitor.hpp"
#include "domain_event_subscriber.hpp"
#include "types.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace sensor::monitor
{
using namespace phosphor::logging;
using json = nlohmann::json;

/**
 * @class RecoverySubscriber
 *
 * This class handles SystemProtectionTriggered and SystemRecoveryTriggered
 * DomainEvent, i.e., start watching specific alarms when system protection
 * is triggered if the config claimed the sensor's recovery action, and stop
 * watching the alarms when system recovery is triggered.
 */
class RecoverySubscriber : public DomainEventSubscriber
{
  public:
    /**
     * @brief Constructor
     *
     * @param[in] dbusAlarmMonitor - The DbusAlarmMonitor object
     */
    RecoverySubscriber(DbusAlarmMonitor& dbusAlarmMonitor);

    /**
     * @brief Handles SystemProtectionTriggered and SystemRecoveryTriggered
     */
    void handle(DomainEvent& event) override final;

  private:
    /**
     * @brief Obtains the alarmsType that claimed in the recovery config
     *
     * @param[in] sensorPath - The sensor which triggerd system protection
     */
    std::vector<AlarmType> obtainRecoveryTypesOf(std::string sensorPath);

    /**
     * @brief Parses the config file into sensorsRecoveryThresholds object
     *
     * @param[in] recoveryConfig - The nlohmann::json object
     */
    void parse(json& recoveryConfig);

    /**
     * @brief Loads and parses the recovery config
     *
     */
    void loadRecoveryActionConfig();

    /**
     * @brief The DbusAlarmMonitor object
     */
    DbusAlarmMonitor& _dbusAlarmMonitor;

    /**
     * @brief The map which stores the recovery alarms claimed in the
     * recovery config
     */
    std::map<std::string, std::map<std::string, int>>
        _sensorsRecoveryThresholds;

    /**
     * @brief The file name of the recovery config.
     */
    const std::string recoveryConfigName = "recovery-action.json";
};

} // namespace sensor::monitor
