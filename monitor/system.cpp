/**
 * Copyright © 2022 IBM Corporation
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

#include "dbus_paths.hpp"
#include "fan.hpp"
#include "fan_defs.hpp"
#include "tach_sensor.hpp"
#include "trust_manager.hpp"
#include "types.hpp"
#include "utility.hpp"
#ifdef MONITOR_USE_JSON
#include "json_config.hpp"
#include "json_parser.hpp"
#endif

#include "config.h"

#include "hwmon_ffdc.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>

namespace phosphor::fan::monitor
{

using json = nlohmann::json;
using Severity = sdbusplus::xyz::openbmc_project::Logging::server::Entry::Level;

using namespace phosphor::logging;

const std::string System::dumpFile = "/tmp/fan_monitor_dump.json";

System::System(Mode mode, sdbusplus::bus_t& bus,
               const sdeventplus::Event& event) :
    _mode(mode),
    _bus(bus), _event(event),
#ifdef MONITOR_USE_HOST_STATE
    _powerState(std::make_unique<HostPowerState>(
#else
    _powerState(std::make_unique<PGoodState>(
#endif
        bus, std::bind(std::mem_fn(&System::powerStateChanged), this,
                       std::placeholders::_1))),
    _thermalAlert(bus, THERMAL_ALERT_OBJPATH)
{}

void System::start()
{
    namespace match = sdbusplus::bus::match;

    // must be done before service detection
    _inventoryMatch = std::make_unique<sdbusplus::bus::match_t>(
        _bus, match::rules::nameOwnerChanged(util::INVENTORY_SVC),
        std::bind(&System::inventoryOnlineCb, this, std::placeholders::_1));

    bool invServiceRunning = util::SDBusPlus::callMethodAndRead<bool>(
        _bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
        "org.freedesktop.DBus", "NameHasOwner", util::INVENTORY_SVC);

    if (invServiceRunning)
    {
        _inventoryMatch.reset();

        if (!_loaded)
        {
            load();
        }
    }
}

void System::load()
{
    json jsonObj = json::object();
#ifdef MONITOR_USE_JSON
    try
    {
        jsonObj = getJsonObj();
#endif
        auto trustGrps = getTrustGroups(jsonObj);
        auto fanDefs = getFanDefinitions(jsonObj);
        // Retrieve and set trust groups within the trust manager
        setTrustMgr(getTrustGroups(jsonObj));
        // Clear/set configured fan definitions
        _fans.clear();
        _fanHealth.clear();
        // Retrieve fan definitions and create fan objects to be monitored
        setFans(fanDefs);
        setFaultConfig(jsonObj);
        log<level::INFO>("Configuration loaded");

        _loaded = true;
#ifdef MONITOR_USE_JSON
    }
    catch (const phosphor::fan::NoConfigFound&)
    {}
#endif

    if (_powerState->isPowerOn())
    {
        // Fans could be missing on startup, so check the power off rules.
        // Tach sensors default to functional, so they wouldn't cause a power
        // off here.
        std::for_each(_powerOffRules.begin(), _powerOffRules.end(),
                      [this](auto& rule) {
                          rule->check(PowerRuleState::runtime, _fanHealth);
                      });
    }

    subscribeSensorsToServices();
}

void System::subscribeSensorsToServices()
{
    namespace match = sdbusplus::bus::match;

    _sensorMatch.clear();

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
            _sensorMatch.emplace_back(std::make_unique<sdbusplus::bus::match_t>(
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

void System::inventoryOnlineCb(sdbusplus::message_t& msg)
{
    namespace match = sdbusplus::bus::match;

    std::string iface;
    msg.read(iface);

    if (util::INVENTORY_INTF != iface)
    {
        return;
    }

    std::string oldName;
    msg.read(oldName);

    std::string newName;
    msg.read(newName);

    // newName should never be empty since match was reset on the first
    // nameOwnerChanged signal received from the service.
    if (!_loaded && !newName.empty())
    {
        load();
    }

    // cancel any further notifications about the service state
    _inventoryMatch.reset();
}

void System::sighupHandler(sdeventplus::source::Signal&,
                           const struct signalfd_siginfo*)
{
    try
    {
        load();
    }
    catch (std::runtime_error& re)
    {
        log<level::ERR>("Error reloading config, no config changes made",
                        entry("LOAD_ERROR=%s", re.what()));
    }
}

const std::vector<CreateGroupFunction>
    System::getTrustGroups([[maybe_unused]] const json& jsonObj)
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

const std::vector<FanDefinition>
    System::getFanDefinitions([[maybe_unused]] const json& jsonObj)
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
void System::tachSignalOffline(sdbusplus::message_t& msg,
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

void System::setFaultConfig([[maybe_unused]] const json& jsonObj)
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
        if (!_loaded)
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
#if DELAY_HOST_CONTROL > 0
            sleep(DELAY_HOST_CONTROL);
            std::for_each(_fans.begin(), _fans.end(),
                          [powerStateOn](auto& fan) {
                              fan->powerStateChanged(powerStateOn);
                          });
            if (std::all_of(_fans.begin(), _fans.end(), [](const auto& fan) {
                    return fan->numSensorsOnDBusAtPowerOn() == 0;
                }))
            {
                handleOfflineFanController();
                return;
            }
#else
            handleOfflineFanController();
            return;
#endif
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
            values["in_range"] = !fan->outOfRange(*sensor);
            values["tach"] = sensor->getInput();

            if (sensor->hasTarget())
            {
                values["target"] = sensor->getTarget();
            }

            // convert between string/json to remove newlines
            values["prev_tachs"] = json(sensor->getPrevTach()).dump();

            if (sensor->hasTarget())
            {
                values["prev_targets"] = json(sensor->getPrevTarget()).dump();
            }

            if (sensor->getMethod() == MethodMode::count)
            {
                values["ticks"] = sensor->getCounter();
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

    createBmcDump();
}

/**
 * @brief Create a BMC Dump
 */
void System::createBmcDump() const
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
        getLogger().log(
            fmt::format("Caught exception while creating BMC dump: {}",
                        e.what()),
            Logger::error);
    }
}

void System::dumpDebugData(sdeventplus::source::Signal&,
                           const struct signalfd_siginfo*)
{
    json output;

    if (_loaded)
    {
        output["logs"] = getLogger().getLogs();
        output["sensors"] = captureSensorData();
    }
    else
    {
        output["error"] = "Fan monitor not loaded yet.  Try again later.";
    }

    std::ofstream file{System::dumpFile};
    if (!file)
    {
        log<level::ERR>("Could not open file for fan monitor dump");
    }
    else
    {
        file << std::setw(4) << output;
    }
}

} // namespace phosphor::fan::monitor
