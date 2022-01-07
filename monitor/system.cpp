/**
 * Copyright © 2021 IBM Corporation
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
#include "system.hpp"

#include "fan.hpp"
#include "fan_defs.hpp"
#include "sdeventplus.hpp"
#include "tach_sensor.hpp"
#include "trust_manager.hpp"
#include "types.hpp"
#include "utility.hpp"
#ifdef MONITOR_USE_JSON
#include "json_parser.hpp"
#endif

#include "config.h"

#include "hwmon_ffdc.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>

namespace phosphor::fan::monitor
{

using json = nlohmann::json;
using Severity = sdbusplus::xyz::openbmc_project::Logging::server::Entry::Level;

using namespace phosphor::logging;

System::System(Mode mode, sdbusplus::bus::bus& bus,
               const sdeventplus::Event& event) :
    _mode(mode),
    _bus(bus), _event(event),
    _powerState(std::make_unique<PGoodState>(
        bus, std::bind(std::mem_fn(&System::powerStateChanged), this,
                       std::placeholders::_1))),
    _thermalAlert(bus, THERMAL_ALERT_OBJPATH),
    _hwmonRetryTimer(util::SDEventPlus::getEvent(),
                     std::bind(&System::hwmonCallback, this))
{}

void System::start()
{
    _started = true;

    json jsonObj = json::object();
#ifdef MONITOR_USE_JSON
    auto confFile =
        fan::JsonConfig::getConfFile(_bus, confAppName, confFileName);
    jsonObj = fan::JsonConfig::load(confFile);
#endif
    // Retrieve and set trust groups within the trust manager
    setTrustMgr(getTrustGroups(jsonObj));
    // Retrieve fan definitions and create fan objects to be monitored
    setFans(getFanDefinitions(jsonObj));
    setFaultConfig(jsonObj);
    log<level::INFO>("Configuration loaded");

    if (_powerState->isPowerOn())
    {
        std::for_each(_powerOffRules.begin(), _powerOffRules.end(),
                      [this](auto& rule) {
                          rule->check(PowerRuleState::runtime, _fanHealth);
                      });
    }

    // start hwmon driver detection
    hwmonCallback();
}

bool System::subscribeHwmon()
{
    namespace match = sdbusplus::bus::match;

    std::set<std::string> svcNames, fanPaths, ifaces;

    // collect a list of all unique fan interfaces
    for (const auto& fan : _fans)
    {
        for (const auto& sensor : fan->sensors())
        {
            fanPaths.insert(sensor->name());
            auto iface(sensor->getInterface());
            if (!iface.empty())
            {
                ifaces.insert(iface);
            }
        }
    }

    ifaces.insert(util::FAN_SENSOR_VALUE_INTF);
    std::vector<std::string> ifacesVec(ifaces.begin(), ifaces.end());

    try
    {
        // for each fan interface, record its service(s)
        for (auto& path : util::SDBusPlus::getSubTreeRaw(_bus, FAN_SENSOR_PATH,
                                                         ifacesVec, 0))
        {
            auto pathItr = fanPaths.find(path.first);

            // mark that the path was inspected.
            if (fanPaths.end() != pathItr)
            {
                fanPaths.erase(pathItr);
            }
            else
            {
                // a fan was found that we are not configured to use.
                getLogger().log(
                    fmt::format(
                        "Unconfigured fan path found: {}, services not added ",
                        path.first),
                    Logger::info);
            }

            for (auto& service : path.second)
            {
                svcNames.insert(service.first);
            }
        }
    }
    catch (std::exception& e)
    {
        getLogger().log(
            fmt::format("Caught D-Bus exception during getSubTreeRaw(): {}",
                        e.what(), Logger::error));

        return false;
    }

    // log any registered fans that do not publish to D-Bus
    if (fanPaths.size() > 0)
    {
        getLogger().log(fmt::format(
            "Fan(s) configured but not found in D-Bus: {}",
            std::accumulate(fanPaths.begin(), fanPaths.end(), std::string("")),
            Logger::error));
    }

    bool svcRunning = false;

    // register the match callback for each service
    for (const auto& svcName : svcNames)
    {
        try
        {
            svcRunning = util::SDBusPlus::callMethodAndRead<bool>(
                _bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
                "org.freedesktop.DBus", "NameHasOwner", svcName);

            // register a callback for each unique service
            // true indicates that the service went offline
            _hwmonMatches.push_back(std::make_unique<match::match>(
                _bus, match::rules::nameOwnerChanged(svcName),
                [=](const auto& unused) { hwmonCallback(); }));
        }
        catch (std::exception& e)
        {
            getLogger().log(fmt::format(
                "Caught D-Bus exception during callMethodAndRead(): {}",
                e.what(), Logger::error));
            svcRunning = false;
        }

        if (!svcRunning)
        {
            // clear CB and force false return val
            _hwmonMatches.clear();
            break;
        }
    }

    // true return value means ok to fully startup
    return svcRunning && (_hwmonMatches.size() == svcNames.size());
}

void System::subscribeSensorsToServices()
{
    namespace match = sdbusplus::bus::match;

    SensorMapType sensorMap;

    // build a list of all interfaces, always including the value interface
    // using set automatically guards against duplicates
    std::set<std::string> unique_interfaces{util::FAN_SENSOR_VALUE_INTF};

    for (const auto& fan : _fans)
    {
        for (const auto& sensor : fan->sensors())
        {
            unique_interfaces.insert(sensor->getInterface());
        }
    }
    // convert them to vector to pass into getSubTreeRaw
    std::vector<std::string> interfaces(unique_interfaces.begin(),
                                        unique_interfaces.end());

    try
    {
        // get service information for all service names that are
        // hosting these interfaces
        auto serviceObjects = util::SDBusPlus::getSubTreeRaw(
            _bus, FAN_SENSOR_PATH, interfaces, 0);

        for (const auto& fan : _fans)
        {
            // For every sensor in each fan
            for (const auto& sensor : fan->sensors())
            {
                const auto itServ = serviceObjects.find(sensor->name());

                if (serviceObjects.end() == itServ || itServ->second.empty())
                {
                    getLogger().log(
                        fmt::format("Fan sensor entry {} not found in D-Bus",
                                    sensor->name()),
                        Logger::error);
                    continue;
                }

                for (const auto& [serviceName, unused] : itServ->second)
                {
                    // associate service name with sensor
                    sensorMap[serviceName].insert(sensor);
                }
            }
        }

        // only create 1 match per service
        for (const auto& [serviceName, unused] : sensorMap)
        {
            // map its service name to the sensor
            _sensorMatch.emplace_back(std::make_unique<match::match>(
                _bus, match::rules::nameOwnerChanged(serviceName),
                std::bind(&System::tachSignalOffline, this,
                          std::placeholders::_1, sensorMap)));
        }
    }
    catch (const util::DBusError&)
    {
        // catch exception from getSubTreeRaw() when fan sensor paths don't
        // exist yet
    }
}

void System::sighupHandler(sdeventplus::source::Signal&,
                           const struct signalfd_siginfo*)
{
    try
    {
        json jsonObj = json::object();
#ifdef MONITOR_USE_JSON
        jsonObj = getJsonObj(_bus);
#endif
        auto trustGrps = getTrustGroups(jsonObj);
        auto fanDefs = getFanDefinitions(jsonObj);
        // Set configured trust groups
        setTrustMgr(trustGrps);
        // Clear/set configured fan definitions
        _fans.clear();
        _fanHealth.clear();
        setFans(fanDefs);
        setFaultConfig(jsonObj);
        log<level::INFO>("Configuration reloaded successfully");

        if (_powerState->isPowerOn())
        {
            std::for_each(_powerOffRules.begin(), _powerOffRules.end(),
                          [this](auto& rule) {
                              rule->check(PowerRuleState::runtime, _fanHealth);
                          });
        }

        _sensorMatch.clear();
        subscribeSensorsToServices();
    }
    catch (const std::runtime_error& re)
    {
        log<level::ERR>("Error reloading config, no config changes made",
                        entry("LOAD_ERROR=%s", re.what()));
    }
}

void System::hwmonCallback()
{
    _hwmonMatches.clear();
    _sensorMatch.clear();

    if (subscribeHwmon())
    {
        // hwMon is running and this cb is registered
        subscribeSensorsToServices();
        _hwmonRetryTimer.setEnabled(false);
    }
    else
    {
        // hwMon not running. activate this function again in a few seconds
        _hwmonRetryTimer.restartOnce(std::chrono::seconds(2));
    }
}

const std::vector<CreateGroupFunction>
    System::getTrustGroups(const json& jsonObj)
{
#ifdef MONITOR_USE_JSON
    return getTrustGrps(jsonObj);
#else
    return trustGroups;
#endif
}

void System::setTrustMgr(const std::vector<CreateGroupFunction>& groupFuncs)
{
    _trust = std::make_unique<trust::Manager>(groupFuncs);
}

const std::vector<FanDefinition> System::getFanDefinitions(const json& jsonObj)
{
#ifdef MONITOR_USE_JSON
    return getFanDefs(jsonObj);
#else
    return fanDefinitions;
#endif
}

void System::setFans(const std::vector<FanDefinition>& fanDefs)
{
    for (const auto& fanDef : fanDefs)
    {
        // Check if a condition exists on the fan
        auto condition = std::get<conditionField>(fanDef);
        if (condition)
        {
            // Condition exists, skip adding fan if it fails
            if (!(*condition)(_bus))
            {
                continue;
            }
        }
        _fans.emplace_back(
            std::make_unique<Fan>(_mode, _bus, _event, _trust, fanDef, *this));

        updateFanHealth(*(_fans.back()));
    }
}

// callback indicating a service went [on|off]line.
// Determine on/offline status, set all sensors for that service
// to new state
//
void System::tachSignalOffline(sdbusplus::message::message& msg,
                               SensorMapType const& sensorMap)
{
    std::string serviceName, oldOwner, newOwner;

    msg.read(serviceName);
    msg.read(oldOwner);
    msg.read(newOwner);

    // true if sensor server came back online, false -> went offline
    bool hasOwner = !newOwner.empty() && oldOwner.empty();

    std::string stateStr(hasOwner ? "online" : "offline");
    getLogger().log(fmt::format("Changing sensors for service {} to {}",
                                serviceName, stateStr),
                    Logger::info);

    auto sensorItr(sensorMap.find(serviceName));

    if (sensorItr != sensorMap.end())
    {
        // set all sensors' owner state to not-owned
        for (auto& sensor : sensorItr->second)
        {
            sensor->setOwner(hasOwner);
            sensor->getFan().process(*sensor);
        }
    }
}

void System::updateFanHealth(const Fan& fan)
{
    std::vector<bool> sensorStatus;
    for (const auto& sensor : fan.sensors())
    {
        sensorStatus.push_back(sensor->functional());
    }

    _fanHealth[fan.getName()] =
        std::make_tuple(fan.present(), std::move(sensorStatus));
}

void System::fanStatusChange(const Fan& fan, bool skipRulesCheck)
{
    updateFanHealth(fan);

    if (_powerState->isPowerOn() && !skipRulesCheck)
    {
        std::for_each(_powerOffRules.begin(), _powerOffRules.end(),
                      [this](auto& rule) {
                          rule->check(PowerRuleState::runtime, _fanHealth);
                      });
    }
}

void System::setFaultConfig(const json& jsonObj)
{
#ifdef MONITOR_USE_JSON
    std::shared_ptr<PowerInterfaceBase> powerInterface =
        std::make_shared<PowerInterface>(_thermalAlert);

    PowerOffAction::PrePowerOffFunc func =
        std::bind(std::mem_fn(&System::logShutdownError), this);

    _powerOffRules = getPowerOffRules(jsonObj, powerInterface, func);

    _numNonfuncSensorsBeforeError = getNumNonfuncRotorsBeforeError(jsonObj);
#endif
}

void System::powerStateChanged(bool powerStateOn)
{
    std::for_each(_fans.begin(), _fans.end(), [powerStateOn](auto& fan) {
        fan->powerStateChanged(powerStateOn);
    });

    if (powerStateOn)
    {
        if (!_started)
        {
            log<level::ERR>("No conf file found at power on");
            throw std::runtime_error("No conf file found at power on");
        }

        // If no fan has its sensors on D-Bus, then there is a problem
        // with the fan controller.  Log an error and shut down.
        if (std::all_of(_fans.begin(), _fans.end(), [](const auto& fan) {
                return fan->numSensorsOnDBusAtPowerOn() == 0;
            }))
        {
            handleOfflineFanController();
            return;
        }

        if (_sensorMatch.empty())
        {
            subscribeSensorsToServices();
        }

        std::for_each(_powerOffRules.begin(), _powerOffRules.end(),
                      [this](auto& rule) {
                          rule->check(PowerRuleState::atPgood, _fanHealth);
                      });
        std::for_each(_powerOffRules.begin(), _powerOffRules.end(),
                      [this](auto& rule) {
                          rule->check(PowerRuleState::runtime, _fanHealth);
                      });
    }
    else
    {
        _thermalAlert.enabled(false);

        // Cancel any in-progress power off actions
        std::for_each(_powerOffRules.begin(), _powerOffRules.end(),
                      [this](auto& rule) { rule->cancel(); });
    }
}

void System::sensorErrorTimerExpired(const Fan& fan, const TachSensor& sensor)
{
    std::string fanPath{util::INVENTORY_PATH + fan.getName()};

    getLogger().log(
        fmt::format("Creating event log for faulted fan {} sensor {}", fanPath,
                    sensor.name()),
        Logger::error);

    // In order to know if the event log should have a severity of error or
    // informational, count the number of existing nonfunctional sensors and
    // compare it to _numNonfuncSensorsBeforeError.
    size_t nonfuncSensors = 0;
    for (const auto& fan : _fans)
    {
        for (const auto& s : fan->sensors())
        {
            // Don't count nonfunctional sensors that still have their
            // error timer running as nonfunctional since they haven't
            // had event logs created for those errors yet.
            if (!s->functional() && !s->errorTimerRunning())
            {
                nonfuncSensors++;
            }
        }
    }

    Severity severity = Severity::Error;
    if (nonfuncSensors < _numNonfuncSensorsBeforeError)
    {
        severity = Severity::Informational;
    }

    auto error =
        std::make_unique<FanError>("xyz.openbmc_project.Fan.Error.Fault",
                                   fanPath, sensor.name(), severity);

    auto sensorData = captureSensorData();
    error->commit(sensorData);

    // Save the error so it can be committed again on a power off.
    _lastError = std::move(error);
}

void System::fanMissingErrorTimerExpired(const Fan& fan)
{
    std::string fanPath{util::INVENTORY_PATH + fan.getName()};

    getLogger().log(
        fmt::format("Creating event log for missing fan {}", fanPath),
        Logger::error);

    auto error = std::make_unique<FanError>(
        "xyz.openbmc_project.Fan.Error.Missing", fanPath, "", Severity::Error);

    auto sensorData = captureSensorData();
    error->commit(sensorData);

    // Save the error so it can be committed again on a power off.
    _lastError = std::move(error);
}

void System::logShutdownError()
{
    if (_lastError)
    {
        getLogger().log("Re-committing previous fan error before power off");

        // Still use the latest sensor data
        auto sensorData = captureSensorData();
        _lastError->commit(sensorData, true);
    }
}

json System::captureSensorData()
{
    json data;

    for (const auto& fan : _fans)
    {
        for (const auto& sensor : fan->sensors())
        {
            json values;
            values["present"] = fan->present();
            values["functional"] = sensor->functional();
            values["tach"] = sensor->getInput();
            if (sensor->hasTarget())
            {
                values["target"] = sensor->getTarget();
            }

            data["sensors"][sensor->name()] = values;
        }
    }

    return data;
}

void System::handleOfflineFanController()
{
    getLogger().log("The fan controller appears to be offline.  Shutting down.",
                    Logger::error);

    auto ffdc = collectHwmonFFDC();

    FanError error{"xyz.openbmc_project.Fan.Error.FanControllerOffline",
                   Severity::Critical};
    error.commit(ffdc, true);

    PowerInterface::executeHardPowerOff();
}

} // namespace phosphor::fan::monitor
