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

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>

namespace phosphor::fan::monitor
{

using json = nlohmann::json;
using namespace phosphor::logging;

System::System(Mode mode, sdbusplus::bus::bus& bus,
               const sdeventplus::Event& event) :
    _mode(mode),
    _bus(bus), _event(event)
{
    _powerState = std::make_unique<PGoodState>(
        bus, std::bind(std::mem_fn(&System::powerStateChanged), this,
                       std::placeholders::_1));

    json jsonObj = json::object();
#ifdef MONITOR_USE_JSON
    jsonObj = getJsonObj(bus);
#endif
    // Retrieve and set trust groups within the trust manager
    setTrustMgr(getTrustGroups(jsonObj));
    // Retrieve fan definitions and create fan objects to be monitored
    setFans(getFanDefinitions(jsonObj));
    setFaultConfig(jsonObj);
    log<level::INFO>("Configuration loaded");

    // Since this doesn't run at standby yet, powerStateChanged
    // will never be called so for now treat start up as the
    // pgood.  When this does run at standby, the 'atPgood'
    // rules won't need to be checked here.
    if (_powerState->isPowerOn())
    {
        std::for_each(_powerOffRules.begin(), _powerOffRules.end(),
                      [this](auto& rule) {
                          rule->check(PowerRuleState::atPgood, _fanHealth);
                      });
        // Runtime rules still need to be checked since fans may already
        // be missing that could trigger a runtime rule.
        std::for_each(_powerOffRules.begin(), _powerOffRules.end(),
                      [this](auto& rule) {
                          rule->check(PowerRuleState::runtime, _fanHealth);
                      });
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

void System::fanStatusChange(const Fan& fan)
{
    updateFanHealth(fan);

    if (_powerState->isPowerOn())
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
        std::make_shared<PowerInterface>();

    _powerOffRules = getPowerOffRules(jsonObj, powerInterface);
#endif
}

void System::powerStateChanged(bool newPowerState)
{
    if (newPowerState)
    {
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
        // Cancel any in-progress power off actions
        std::for_each(_powerOffRules.begin(), _powerOffRules.end(),
                      [this](auto& rule) { rule->cancel(); });
    }
}

} // namespace phosphor::fan::monitor
