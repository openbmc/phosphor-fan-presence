#include "config.h"

#include "alarm_handlers.hpp"

#include "alarm_timestamps.hpp"
#include "domain_event_publisher.hpp"
#include "domain_events.hpp"

#include <typeinfo>

namespace sensor::monitor
{
using namespace phosphor::logging;
using namespace phosphor::fan::util;
using namespace phosphor::fan;
using json = nlohmann::json;

const std::map<AlarmType, std::chrono::milliseconds> shutdownDelays{
    {AlarmType::hardShutdown,
     std::chrono::milliseconds{SHUTDOWN_ALARM_HARD_SHUTDOWN_DELAY_MS}},
    {AlarmType::softShutdown,
     std::chrono::milliseconds{SHUTDOWN_ALARM_SOFT_SHUTDOWN_DELAY_MS}}};

const std::map<AlarmType, std::map<AlarmDirection, std::string>>
    alarmProperties{{AlarmType::hardShutdown,
                     {{AlarmDirection::low, "HardShutdownAlarmLow"},
                      {AlarmDirection::high, "HardShutdownAlarmHigh"}}},
                    {AlarmType::softShutdown,
                     {{AlarmDirection::low, "SoftShutdownAlarmLow"},
                      {AlarmDirection::high, "SoftShutdownAlarmHigh"}}},
                    {AlarmType::critical,
                     {{AlarmDirection::low, "CriticalAlarmLow"},
                      {AlarmDirection::high, "CriticalAlarmHigh"}}},
                    {AlarmType::warning,
                     {{AlarmDirection::low, "WarningAlarmLow"},
                      {AlarmDirection::high, "WarningAlarmHigh"}}}};

const std::map<AlarmType, std::map<AlarmDirection, std::string>> alarmEventLogs{
    {AlarmType::hardShutdown,
     {{AlarmDirection::high,
       "xyz.openbmc_project.Sensor.Threshold.Error.HardShutdownAlarmHigh"},
      {AlarmDirection::low, "xyz.openbmc_project.Sensor.Threshold.Error."
                            "HardShutdownAlarmLow"}}},
    {AlarmType::softShutdown,
     {{AlarmDirection::high,
       "xyz.openbmc_project.Sensor.Threshold.Error.SoftShutdownAlarmHigh"},
      {AlarmDirection::low, "xyz.openbmc_project.Sensor.Threshold.Error."
                            "SoftShutdownAlarmLow"}}}};

const std::map<AlarmType, std::map<AlarmDirection, std::string>>
    alarmClearEventLogs{
        {AlarmType::hardShutdown,
         {{AlarmDirection::high, "xyz.openbmc_project.Sensor.Threshold.Error."
                                 "HardShutdownAlarmHighClear"},
          {AlarmDirection::low, "xyz.openbmc_project.Sensor.Threshold.Error."
                                "HardShutdownAlarmLowClear"}}},
        {AlarmType::softShutdown,
         {{AlarmDirection::high, "xyz.openbmc_project.Sensor.Threshold.Error."
                                 "SoftShutdownAlarmHighClear"},
          {AlarmDirection::low, "xyz.openbmc_project.Sensor.Threshold.Error."
                                "SoftShutdownAlarmLowClear"}}}};

const auto loggingService = "xyz.openbmc_project.Logging";
const auto loggingPath = "/xyz/openbmc_project/logging";
const auto loggingCreateIface = "xyz.openbmc_project.Logging.Create";
constexpr auto systemdService = "org.freedesktop.systemd1";
constexpr auto systemdPath = "/org/freedesktop/systemd1";
constexpr auto systemdMgrIface = "org.freedesktop.systemd1.Manager";
constexpr auto valueInterface = "xyz.openbmc_project.Sensor.Value";
constexpr auto valueProperty = "Value";
constexpr auto recoveryConfigName = "recovery-action.json";

std::string target = "";

void ProtectionAlarmHandler::startTimer(const AlarmKey& alarmKey)
{
    const auto& [sensorPath, alarmType, alarmDirection] = alarmKey;
    const auto& propertyName = alarmProperties.at(alarmType).at(alarmDirection);
    std::chrono::milliseconds shutdownDelay{shutdownDelays.at(alarmType)};
    std::optional<double> value;

    auto alarm = _alarms.find(alarmKey);
    if (alarm == _alarms.end())
    {
        throw std::runtime_error("Couldn't find alarm inside startTimer");
    }

    try
    {
        log<level::INFO>("Get sensor value in startTimer()");
        value = SDBusPlus::getProperty<double>(_bus, sensorPath, valueInterface,
                                               valueProperty);
    }
    catch (const DBusServiceError& e)
    {
        log<level::ERR>(
            fmt::format("Failed to obtain sesor's value: {}", e.what())
                .c_str());
    }

    createEventLog(alarmKey, true, value);

    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();

    log<level::INFO>(
        fmt::format("Starting {}ms {} shutdown timer due to sensor {} value {}",
                    shutdownDelay.count(), propertyName, sensorPath, *value)
            .c_str());

    auto& timer = alarm->second;

    timer = std::make_unique<
        sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>>(
        _event,
        std::bind(&ProtectionAlarmHandler::triggerProtection, this, alarmKey));

    timer->restartOnce(shutdownDelay);

    AlarmTimestamps::instance().add(alarmKey, now);
}

void ProtectionAlarmHandler::stopTimer(const AlarmKey& alarmKey)
{
    const auto& [sensorPath, alarmType, alarmDirection] = alarmKey;
    const auto& propertyName = alarmProperties.at(alarmType).at(alarmDirection);

    auto sensorValue = SDBusPlus::getProperty<double>(
        _bus, sensorPath, valueInterface, valueProperty);

    auto alarm = _alarms.find(alarmKey);
    if (alarm == _alarms.end())
    {
        throw std::runtime_error("Couldn't find alarm inside stopTimer");
    }

    createEventLog(alarmKey, false, sensorValue);

    log<level::INFO>(
        fmt::format("Stopping {} shutdown timer due to sensor {} value {}",
                    propertyName, sensorPath, sensorValue)
            .c_str());

    auto& timer = alarm->second;
    timer->setEnabled(false);
    timer.reset();

    AlarmTimestamps::instance().erase(alarmKey);
}

bool ProtectionAlarmHandler::isBackToNormal(const bool hasTimerBeenStarted,
                                            const bool isAlarmTriggered)
{
    return hasTimerBeenStarted && !isAlarmTriggered;
}

void ProtectionAlarmHandler::check(bool isAlarmTriggered,
                                   const AlarmKey& alarmKey)
{
    auto alarm = _alarms.find(alarmKey);
    if (alarm == _alarms.end())
    {
        return;
    }

    const bool hasTimerBeenStarted = alarm->second ? true : false;
    if (isAlarmTriggered && !hasTimerBeenStarted)
    {
        startTimer(alarmKey);
    }

    if (isBackToNormal(hasTimerBeenStarted, isAlarmTriggered))
    {
        stopTimer(alarmKey);
    }

    return;
}

void ProtectionAlarmHandler::createBmcDump() const
{
    try
    {
        util::SDBusPlus::callMethod(
            "xyz.openbmc_project.Dump.Manager", "/xyz/openbmc_project/dump/bmc",
            "xyz.openbmc_project.Dump.Create", "CreateDump",
            std::vector<
                std::pair<std::string, std::variant<std::string, uint64_t>>>());
    }
    catch (const std::exception& e)
    {
        auto message = fmt::format(
            "Caught exception while creating BMC dump: {}", e.what());

        log<level::ERR>(message.c_str());
    }

    return;
}

void ProtectionAlarmHandler::triggerProtection(const AlarmKey& alarmKey)
{
    const auto& [sensorPath, alarmType, alarmDirection] = alarmKey;
    const auto& propertyName = alarmProperties.at(alarmType).at(alarmDirection);

    auto sensorValue = SDBusPlus::getProperty<double>(
        _bus, sensorPath, valueInterface, valueProperty);

    log<level::ERR>(
        fmt::format(
            "The {} shutdown timer expired for sensor {}, shutting down",
            propertyName, sensorPath)
            .c_str());

    try
    {
        SDBusPlus::callMethod(systemdService, systemdPath, systemdMgrIface,
                              "StartUnit",
                              "obmc-chassis-hard-poweroff@0.target", "replace");
    }
    catch (const DBusMethodError& e)
    {
        log<level::ERR>(
            fmt::format(
                "Failed to execute Protection target, error message: {}",
                e.what())
                .c_str());
        return;
    }

    // Re-send the event log.  If someone didn't want this it could be
    // wrapped by a compile option.
    createEventLog(alarmKey, true, sensorValue, true);

    AlarmTimestamps::instance().erase(alarmKey);
    createBmcDump();

    SensorProtectionTriggered protectionTriggered(sensorPath);
    DomainEventPublisher::instance().publish(protectionTriggered);

    return;
}

void ProtectionAlarmHandler::checkAlarm(
    std::string sensorPath,
    std::map<std::string, std::variant<bool>> properties)
{
    if (!_powerState->isPowerOn())
    {
        return;
    }

    auto getPropertyValue = [&properties](std::string property) {
        return std::get<bool>(properties.at(property));
    };

    const auto& lowAlarmName =
        alarmProperties.at(_handledType).at(AlarmDirection::low);
    if (properties.count(lowAlarmName) > 0)
    {
        AlarmKey alarmKey{sensorPath, _handledType, AlarmDirection::low};
        auto alarm = _alarms.find(alarmKey);
        if (alarm == _alarms.end())
        {
            _alarms.emplace(alarmKey, nullptr);
        }

        check(getPropertyValue(lowAlarmName), alarmKey);
    }

    const auto& highAlarmName =
        alarmProperties.at(_handledType).at(AlarmDirection::high);
    if (properties.count(highAlarmName) > 0)
    {
        AlarmKey alarmKey{sensorPath, _handledType, AlarmDirection::high};
        auto alarm = _alarms.find(alarmKey);
        if (alarm == _alarms.end())
        {
            _alarms.emplace(alarmKey, nullptr);
        }

        check(getPropertyValue(highAlarmName), alarmKey);
    }
}

void ProtectionAlarmHandler::createEventLog(
    const AlarmKey& alarmKey, bool alarmValue,
    const std::optional<double>& sensorValue, bool isPowerOffError)
{
    using namespace sdbusplus::xyz::openbmc_project::Logging::server;
    const auto& [sensorPath, alarmType, alarmDirection] = alarmKey;
    std::map<std::string, std::string> ad{{"SENSOR_NAME", sensorPath},
                                          {"_PID", std::to_string(getpid())}};

    std::string errorName =
        (alarmValue) ? alarmEventLogs.at(alarmType).at(alarmDirection)
                     : alarmClearEventLogs.at(alarmType).at(alarmDirection);

    // severity = Critical if a power off
    // severity = Error if alarm was asserted
    // severity = Informational if alarm was deasserted
    Entry::Level severity = Entry::Level::Error;
    if (isPowerOffError)
    {
        severity = Entry::Level::Critical;
    }
    else if (!alarmValue)
    {
        severity = Entry::Level::Informational;
    }

    if (sensorValue)
    {
        ad.emplace("SENSOR_VALUE", std::to_string(*sensorValue));
    }

    // If this is a power off, specify that it's a power
    // fault and a system termination.  This is used by some
    // implementations for service reasons.
    if (isPowerOffError)
    {
        ad.emplace("SEVERITY_DETAIL", "SYSTEM_TERM");
    }

    try
    {
        SDBusPlus::callMethod(loggingService, loggingPath, loggingCreateIface,
                              "Create", errorName, convertForMessage(severity),
                              ad);
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(
            fmt::format("Failed to create event log: {}", e.what()).c_str());
    }

    return;
}

RecoveryAlarmHandler::RecoveryAlarmHandler(
    std::map<AlarmKey, std::unique_ptr<sdeventplus::utility::Timer<
                           sdeventplus::ClockId::Monotonic>>>& alarms,
    sdbusplus::bus_t& bus, sdeventplus::Event& event, AlarmType type) :
    AlarmHandler(alarms, bus),
    _event(event), _handledType(type)
{}

void RecoveryAlarmHandler::triggerRecovery(const AlarmKey& alarmKey)
{
    const auto& [sensorPath, alarmType, alarmDirection] = alarmKey;
    const auto& propertyName = alarmProperties.at(alarmType).at(alarmDirection);

    log<level::INFO>(
        fmt::format(
            "The {} recovery timer expired for sensor {}, recovery the system",
            propertyName, sensorPath)
            .c_str());

    log<level::INFO>(fmt::format("Calling target: {}", target).c_str());
    try
    {
        SDBusPlus::callMethod(systemdService, systemdPath, systemdMgrIface,
                              "StartUnit", target, "replace");
    }
    catch (const DBusMethodError& e)
    {
        log<level::ERR>(
            fmt::format("Failed to execute Protection target: {}: {}", target,
                        e.what())
                .c_str());
        return;
    }

    // TODO: Send system recovered event to redfish
    SensorRecoveryTriggered recoveryTriggered(sensorPath, alarmType);
    DomainEventPublisher::instance().publish(recoveryTriggered);

    AlarmTimestamps::instance().erase(alarmKey);
}

void RecoveryAlarmHandler::startTimer(const AlarmKey& alarmKey,
                                      uint16_t timerDelay)
{
    const auto& [sensorPath, alarmType, alarmDirection] = alarmKey;
    const auto& propertyName = alarmProperties.at(alarmType).at(alarmDirection);
    std::chrono::milliseconds countdownTime{timerDelay};

    std::optional<double> sensorValue;

    auto alarm = _alarms.find(alarmKey);
    if (alarm == _alarms.end())
    {
        log<level::ERR>("Couldn't find alarm inside startTimer.");
        return;
    }

    try
    {
        sensorValue = SDBusPlus::getProperty<double>(
            _bus, sensorPath, valueInterface, valueProperty);
    }
    catch (const DBusServiceError& e)
    {
        log<level::ERR>(
            fmt::format("Failed to obtain sesor's value: {}", e.what())
                .c_str());
    }

    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();

    log<level::INFO>(
        fmt::format("Starting {}ms {} recovery timer due to sensor {} value {}",
                    countdownTime.count(), propertyName, sensorPath,
                    *sensorValue)
            .c_str());

    auto& timer = alarm->second;

    timer = std::make_unique<
        sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>>(
        _event,
        std::bind(&RecoveryAlarmHandler::triggerRecovery, this, alarmKey));

    timer->restartOnce(countdownTime);

    AlarmTimestamps::instance().add(alarmKey, now);
}

void RecoveryAlarmHandler::stopTimer(const AlarmKey& alarmKey)
{
    const auto& [sensorPath, alarmType, alarmDirection] = alarmKey;
    const auto& propertyName = alarmProperties.at(alarmType).at(alarmDirection);

    auto sensorValue = SDBusPlus::getProperty<double>(
        _bus, sensorPath, valueInterface, valueProperty);

    auto alarm = _alarms.find(alarmKey);
    if (alarm == _alarms.end())
    {
        throw std::runtime_error("Couldn't find alarm inside stopTimer");
    }

    log<level::INFO>(
        fmt::format("Stopping {} recovery timer due to sensor {} value {}",
                    propertyName, sensorPath, sensorValue)
            .c_str());

    auto& timer = alarm->second;
    timer->setEnabled(false);
    timer.reset();

    AlarmTimestamps::instance().erase(alarmKey);
}

bool RecoveryAlarmHandler::isBackToAbnormal(const bool hasTimerBeenStarted,
                                            const bool isAlarmTriggered)
{
    return hasTimerBeenStarted && isAlarmTriggered;
}

void RecoveryAlarmHandler::check(bool isAlarmTriggered,
                                 const AlarmKey& alarmKey, uint16_t timerDelay)
{
    auto alarm = _alarms.find(alarmKey);
    if (alarm == _alarms.end())
    {
        return;
    }

    const bool hasTimerBeenStarted = alarm->second ? true : false;
    if (!isAlarmTriggered && !hasTimerBeenStarted)
    {
        startTimer(alarmKey, timerDelay);
    }

    if (isBackToAbnormal(hasTimerBeenStarted, isAlarmTriggered))
    {
        stopTimer(alarmKey);
    }

    return;
}

nlohmann::json RecoveryAlarmHandler::loadRecoveryActionConfig()
{
    std::filesystem::path path =
        std::filesystem::path{SENSOR_MONITOR_PERSIST_ROOT_PATH} /
        recoveryConfigName;

    if (!std::filesystem::exists(path))
    {
        throw std::runtime_error(fmt::format("Config file: {}/{} don't exist",
                                             SENSOR_MONITOR_PERSIST_ROOT_PATH,
                                             recoveryConfigName)
                                     .c_str());
    }

    std::ifstream configFile(path.c_str());
    json recoveryConfig = json::parse(configFile);
    if (recoveryConfig.empty())
    {
        configFile.close();
        throw std::runtime_error(fmt::format("Config file: {}/{} is empty",
                                             SENSOR_MONITOR_PERSIST_ROOT_PATH,
                                             recoveryConfigName)
                                     .c_str());
    }
    configFile.close();

    return recoveryConfig;
}

std::map<AlarmDirection, uint16_t>
    RecoveryAlarmHandler::obtainTimerConfigFor(const std::string& sensorPath)
{
    std::map<AlarmDirection, uint16_t> result;
    try
    {
        auto recoveryConfig = loadRecoveryActionConfig();
        target = recoveryConfig.at("target");
        for (auto sensorConfig : recoveryConfig.at("sensors"))
        {
            std::string theSensorPath = sensorConfig.at("path");
            if (theSensorPath == sensorPath)
            {
                std::map<std::string, int> thresholds;
                for (auto threshold : sensorConfig.at("thresholds"))
                {
                    std::string property = threshold.at("alarm");
                    int delay = threshold.at("stableCountdown");
                    thresholds.emplace(property, delay);
                }

                std::map<AlarmDirection, std::string> searchingProperties =
                    alarmProperties.find(_handledType)->second;

                for (auto [direction, property] : searchingProperties)
                {
                    auto it = thresholds.find(property);
                    if (it != thresholds.end())
                    {
                        result.emplace(direction, it->second);
                    }
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(e.what());
    }

    return result;
}

void RecoveryAlarmHandler::checkAlarm(
    std::string sensorPath,
    std::map<std::string, std::variant<bool>> properties)
{
    auto getPropertyValue = [&properties](std::string property) {
        return std::get<bool>(properties.at(property));
    };

    for (auto& [direction, delay] : obtainTimerConfigFor(sensorPath))
    {
        const auto& alarmPropertyName =
            alarmProperties.at(_handledType).at(direction);
        if (properties.count(alarmPropertyName) > 0)
        {
            AlarmKey alarmKey{sensorPath, _handledType, direction};
            auto alarm = _alarms.find(alarmKey);
            if (alarm == _alarms.end())
            {
                _alarms.emplace(alarmKey, nullptr);
            }

            check(getPropertyValue(alarmPropertyName), alarmKey, delay);
        }
    }

    return;
}

} // namespace sensor::monitor
