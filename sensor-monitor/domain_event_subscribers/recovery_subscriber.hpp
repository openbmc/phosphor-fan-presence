#pragma once
#include "config.h"

#include "dbus_alarm_monitor.hpp"
#include "domain_event_subscriber.hpp"
#include "types.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

#include <string>
#include <typeinfo>
#include <vector>

namespace sensor::monitor
{
using namespace phosphor::logging;
using json = nlohmann::json;

class RecoverySubscriber : public DomainEventSubscriber
{
  public:
    RecoverySubscriber(DbusAlarmMonitor& dbusAlarmMonitor) :
        _dbusAlarmMonitor(dbusAlarmMonitor)
    {
        loadRecoveryActionConfig();
    }

    void handle(DomainEvent& event) override final
    {
        if (typeid(event) == typeid(SensorProtectionTriggered))
        {
            auto& theEvent = dynamic_cast<SensorProtectionTriggered&>(event);
            std::string sensorPath = theEvent.getSensorPath();
            for (auto& recoveryType : obtainRecoveryTypesOf(sensorPath))
            {
                _dbusAlarmMonitor.watchSensorAlarm(sensorPath, recoveryType);
            }
        }
        else if (typeid(event) == typeid(SensorRecoveryTriggered))
        {
            auto& theEvent = dynamic_cast<SensorRecoveryTriggered&>(event);
            _dbusAlarmMonitor.stopWatchAlarm(theEvent.getSensorPath(),
                                             theEvent.getAlarmType());
        }
    }

  private:
    std::vector<AlarmType> obtainRecoveryTypesOf(std::string sensorPath)
    {
        std::vector<AlarmType> result;

        auto it = _sensorsRecoveryThresholds.find(sensorPath);
        if (it == _sensorsRecoveryThresholds.end())
        {
            return result;
        }

        for (auto& [threshold, delay] : it->second)
        {
            if (threshold == "CriticalAlarmLow" ||
                threshold == "CriticalAlarmHigh")
            {
                result.emplace_back(AlarmType::critical);
            }
            else if (threshold == "WarningAlarmLow" ||
                     threshold == "WarningAlarmHigh")
            {
                result.emplace_back(AlarmType::warning);
            }
            else
            {
                log<level::ERR>(fmt::format("{} is not match to any\
              threshold interface",
                                            threshold)
                                    .c_str());
            }
        }

        return result;
    }

    void parse(json& recoveryConfig)
    {
        for (auto& sensorConfig : recoveryConfig.at("sensors"))
        {
            try
            {
                std::string sensorPath = sensorConfig.at("path");
                std::map<std::string, int> thresholds;
                for (auto& threshold : sensorConfig.at("thresholds"))
                {
                    thresholds.emplace(threshold.at("alarm"),
                                       threshold.at("stableCountdown"));
                }

                _sensorsRecoveryThresholds.emplace(sensorPath, thresholds);
            }
            catch (const std::exception& e)
            {
                log<level::ERR>(
                    fmt::format("Failed to parse config file: {}", e.what())
                        .c_str());
            }
        }
    }

    void loadRecoveryActionConfig()
    {
        std::filesystem::path path =
            std::filesystem::path{SENSOR_MONITOR_PERSIST_ROOT_PATH} /
            recoveryConfigName;

        if (!std::filesystem::exists(path))
        {
            log<level::ERR>(fmt::format("Config file: {}/{} don't exist",
                                        SENSOR_MONITOR_PERSIST_ROOT_PATH,
                                        recoveryConfigName)
                                .c_str());
            return;
        }

        std::ifstream configFile(path.c_str());
        json recoveryConfig = json::parse(configFile);
        if (recoveryConfig.empty())
        {
            configFile.close();
            log<level::INFO>("The configuration is empty.");
            return;
        }
        configFile.close();

        parse(recoveryConfig);

        return;
    }

    DbusAlarmMonitor& _dbusAlarmMonitor;

    std::map<std::string, std::map<std::string, int>>
        _sensorsRecoveryThresholds;

    const std::string recoveryConfigName = "recovery-action.json";
};

} // namespace sensor::monitor
