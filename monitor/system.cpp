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
#include "system.hpp"

#include "fan.hpp"
#include "fan_defs.hpp"
#include "tach_sensor.hpp"
#include "trust_manager.hpp"
#include "types.hpp"
#ifdef MONITOR_USE_JSON
#include "json_parser.hpp"
#endif

#include "config.h"

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>

constexpr auto FAN_SENSOR_VALUE_INTF = "xyz.openbmc_project.Sensor.Value";

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
    _thermalAlert(bus, THERMAL_ALERT_OBJPATH)
{}

void System::start(
#ifdef MONITOR_USE_JSON
    const std::string& confFile
#endif
)
{
    _started = true;
    json jsonObj = json::object();
#ifdef MONITOR_USE_JSON
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

    SensorMapType sensor_map;

    // Get all subtree objects for the given interface
    auto objects =
        util::SDBusPlus::getSubTree(_bus, "/", FAN_SENSOR_VALUE_INTF, 0);

    // for each object, lookup its services
    for (auto& pIter : objects)
    {
        auto& sensor_name(pIter.first);

        // For every service that controls fan sensors
        for (auto& sIter : pIter.second)
        {
            auto& service = sIter.first;

            // for all fans
            for (auto& fan2 : _fans)
            {
                // For every sensor in that fan
                for (auto& sensor : fan2->sensors())
                {
                    // select the sensor whose path matches
                    if (sensor->name() == sensor_name)
                    {
                        // 1) mark this sensor to recv callbacks when _sensor_
                        // iface goes offline
                        sensor_map[service].insert(sensor);

                        // 2) if it has target, mark it to recv callbacks when
                        // _target_ iface goes offline
                        if (sensor->hasTarget())
                        {
                            sensor_map[sensor->getInterface()].insert(sensor);
                        }

                        break;
                    }
                }
            }
        }
    }

    namespace rules = sdbusplus::bus::match::rules;

    // for each service, register a callback handler for nameOwnerChanged
    for (auto& service_itr : sensor_map)
    {
        auto& service = service_itr.first;

        try
        {
            _sensorMatch.push_back(
                std::make_unique<sdbusplus::bus::match::match>(
                    _bus, rules::nameOwnerChanged(service),
                    std::bind(&System::tach_signal_offline, this,
                              std::placeholders::_1, sensor_map)));
        }
        catch (const util::DBusError& e)
        {
            // Unable to construct NameOwnerChanged match string
            getLogger().log(
                fmt::format("Exception creating sdbusplus:: callback: {}",
                            e.what()),
                Logger::error);
        }
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
    }
    catch (std::runtime_error& re)
    {
        log<level::ERR>("Error reloading config, no config changes made",
                        entry("LOAD_ERROR=%s", re.what()));
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
void System::tach_signal_offline(sdbusplus::message::message& msg,
                                 SensorMapType const& sensor_map)
{
    std::string service_name, old_owner, new_owner;

    msg.read(service_name);
    msg.read(old_owner);
    msg.read(new_owner);

    // true if sensor server came back online, false -> went offline
    bool on_offline = !new_owner.empty() && old_owner.empty();

    auto sensor_itr(sensor_map.find(service_name));

    if (sensor_itr != sensor_map.end())
    {
        // for all fans under this service
        for (auto& sensor : sensor_itr->second)
        {
            sensor->setOwner(on_offline);
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

} // namespace phosphor::fan::monitor
