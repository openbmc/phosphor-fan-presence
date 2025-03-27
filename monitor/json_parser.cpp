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
#include "json_parser.hpp"

#include "conditions.hpp"
#include "json_config.hpp"
#include "nonzero_speed_trust.hpp"
#include "power_interface.hpp"
#include "power_off_rule.hpp"
#include "tach_sensor.hpp"
#include "types.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/lg2.hpp>

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace phosphor::fan::monitor
{

using json = nlohmann::json;
using namespace phosphor::logging;

namespace tClass
{

// Get a constructed trust group class for a non-zero speed group
CreateGroupFunction getNonZeroSpeed(
    const std::vector<trust::GroupDefinition>& group)
{
    return [group]() {
        return std::make_unique<trust::NonzeroSpeed>(std::move(group));
    };
}

} // namespace tClass

const std::map<std::string, trustHandler> trusts = {
    {"nonzerospeed", tClass::getNonZeroSpeed}};
const std::map<std::string, condHandler> conditions = {
    {"propertiesmatch", condition::getPropertiesMatch}};
const std::map<std::string, size_t> methods = {
    {"timebased", MethodMode::timebased}, {"count", MethodMode::count}};

const std::vector<CreateGroupFunction> getTrustGrps(const json& obj)
{
    std::vector<CreateGroupFunction> grpFuncs;

    if (obj.contains("sensor_trust_groups"))
    {
        for (auto& stg : obj["sensor_trust_groups"])
        {
            if (!stg.contains("class") || !stg.contains("group"))
            {
                // Log error on missing required parameters
                lg2::error(
                    "Missing required fan monitor trust group parameters 'class, group'");
                throw std::runtime_error(
                    "Missing required fan trust group parameters");
            }
            auto tgClass = stg["class"].get<std::string>();
            std::vector<trust::GroupDefinition> group;
            for (auto& member : stg["group"])
            {
                // Construct list of group members
                if (!member.contains("name"))
                {
                    // Log error on missing required parameter
                    lg2::error(
                        "Missing required fan monitor trust group member name for class {CLASS}",
                        "CLASS", tgClass);
                    throw std::runtime_error(
                        "Missing required fan monitor trust group member name");
                }
                auto in_trust = true;
                if (member.contains("in_trust"))
                {
                    in_trust = member["in_trust"].get<bool>();
                }
                group.emplace_back(trust::GroupDefinition{
                    member["name"].get<std::string>(), in_trust});
            }
            // The class for fan sensor trust groups
            // (Must have a supported function within the tClass namespace)
            std::transform(tgClass.begin(), tgClass.end(), tgClass.begin(),
                           tolower);
            auto handler = trusts.find(tgClass);
            if (handler != trusts.end())
            {
                // Call function for trust group class
                grpFuncs.emplace_back(handler->second(group));
            }
            else
            {
                // Log error on unsupported trust group class
                lg2::error("Invalid fan monitor trust group class {CLASS}",
                           "CLASS", tgClass);
                throw std::runtime_error(
                    "Invalid fan monitor trust group class");
            }
        }
    }

    return grpFuncs;
}

const std::vector<SensorDefinition> getSensorDefs(const json& sensors)
{
    std::vector<SensorDefinition> sensorDefs;

    for (const auto& sensor : sensors)
    {
        if (!sensor.contains("name") || !sensor.contains("has_target"))
        {
            // Log error on missing required parameters
            lg2::error(
                "Missing required fan sensor definition parameters 'name, has_target'");
            throw std::runtime_error(
                "Missing required fan sensor definition parameters");
        }
        // Target interface is optional and defaults to
        // 'xyz.openbmc_project.Control.FanSpeed'
        std::string targetIntf = "xyz.openbmc_project.Control.FanSpeed";
        if (sensor.contains("target_interface"))
        {
            targetIntf = sensor["target_interface"].get<std::string>();
        }
        // Target path is optional
        std::string targetPath;
        if (sensor.contains("target_path"))
        {
            targetPath = sensor["target_path"].get<std::string>();
        }
        // Factor is optional and defaults to 1
        double factor = 1.0;
        if (sensor.contains("factor"))
        {
            factor = sensor["factor"].get<double>();
        }
        // Offset is optional and defaults to 0
        int64_t offset = 0;
        if (sensor.contains("offset"))
        {
            offset = sensor["offset"].get<int64_t>();
        }
        // Threshold is optional and defaults to 1
        size_t threshold = 1;
        if (sensor.contains("threshold"))
        {
            threshold = sensor["threshold"].get<size_t>();
        }
        // Ignore being above the allowed max is optional, defaults to not
        bool ignoreAboveMax = false;
        if (sensor.contains("ignore_above_max"))
        {
            ignoreAboveMax = sensor["ignore_above_max"].get<bool>();
        }

        SensorDefinition def{
            .name = sensor["name"].get<std::string>(),
            .hasTarget = sensor["has_target"].get<bool>(),
            .targetInterface = targetIntf,
            .targetPath = targetPath,
            .factor = factor,
            .offset = offset,
            .threshold = threshold,
            .ignoreAboveMax = ignoreAboveMax};

        sensorDefs.push_back(std::move(def));
    }

    return sensorDefs;
}

const std::vector<FanDefinition> getFanDefs(const json& obj)
{
    std::vector<FanDefinition> fanDefs;

    for (const auto& fan : obj["fans"])
    {
        if (!fan.contains("inventory") || !fan.contains("deviation") ||
            !fan.contains("sensors"))
        {
            // Log error on missing required parameters
            lg2::error(
                "Missing required fan monitor definition parameters 'inventory, deviation, sensors'");
            throw std::runtime_error(
                "Missing required fan monitor definition parameters");
        }
        // Valid deviation range is 0 - 100%
        auto deviation = fan["deviation"].get<size_t>();
        if (100 < deviation)
        {
            lg2::error(
                "Invalid deviation of {DEVIATION} found, must be between 0 and 100",
                "DEVIATION", deviation);
            throw std::runtime_error(
                "Invalid deviation found, must be between 0 and 100");
        }

        // Upper deviation defaults to the deviation value and
        // can also be separately specified.
        size_t upperDeviation = deviation;
        if (fan.contains("upper_deviation"))
        {
            upperDeviation = fan["upper_deviation"].get<size_t>();
            if (100 < upperDeviation)
            {
                lg2::error(
                    "Invalid upper_deviation of {UPPER_DEVIATION} found, must be between 0 and 100",
                    "UPPER_DEVIATION", upperDeviation);
                throw std::runtime_error(
                    "Invalid upper_deviation found, must be between 0 and 100");
            }
        }

        // Construct the sensor definitions for this fan
        auto sensorDefs = getSensorDefs(fan["sensors"]);

        // Functional delay is optional and defaults to 0
        size_t funcDelay = 0;
        if (fan.contains("functional_delay"))
        {
            funcDelay = fan["functional_delay"].get<size_t>();
        }

        // Method is optional and defaults to time based functional
        // determination
        size_t method = MethodMode::timebased;
        size_t countInterval = 1;
        if (fan.contains("method"))
        {
            auto methodConf = fan["method"].get<std::string>();
            auto methodFunc = methods.find(methodConf);
            if (methodFunc != methods.end())
            {
                method = methodFunc->second;
            }
            else
            {
                // Log error on unsupported method parameter
                lg2::error("Invalid fan method");
                throw std::runtime_error("Invalid fan method");
            }

            // Read the count interval value used with the count method.
            if (method == MethodMode::count)
            {
                if (fan.contains("count_interval"))
                {
                    countInterval = fan["count_interval"].get<size_t>();
                }
            }
        }

        // Timeout defaults to 0
        size_t timeout = 0;
        if (method == MethodMode::timebased)
        {
            if (!fan.contains("allowed_out_of_range_time"))
            {
                // Log error on missing required parameter
                lg2::error(
                    "Missing required fan monitor definition parameters 'allowed_out_of_range_time'");
                throw std::runtime_error(
                    "Missing required fan monitor definition parameters");
            }
            else
            {
                timeout = fan["allowed_out_of_range_time"].get<size_t>();
            }
        }

        // Monitor start delay is optional and defaults to 0
        size_t monitorDelay = 0;
        if (fan.contains("monitor_start_delay"))
        {
            monitorDelay = fan["monitor_start_delay"].get<size_t>();
        }

        // num_sensors_nonfunc_for_fan_nonfunc is optional and defaults
        // to zero if not present, meaning the code will not set the
        // parent fan to nonfunctional based on sensors.
        size_t nonfuncSensorsCount = 0;
        if (fan.contains("num_sensors_nonfunc_for_fan_nonfunc"))
        {
            nonfuncSensorsCount =
                fan["num_sensors_nonfunc_for_fan_nonfunc"].get<size_t>();
        }

        // nonfunc_rotor_error_delay is optional, though it will
        // default to zero if 'fault_handling' is present.
        std::optional<size_t> nonfuncRotorErrorDelay;
        if (fan.contains("nonfunc_rotor_error_delay"))
        {
            nonfuncRotorErrorDelay =
                fan["nonfunc_rotor_error_delay"].get<size_t>();
        }
        else if (obj.contains("fault_handling"))
        {
            nonfuncRotorErrorDelay = 0;
        }

        // fan_missing_error_delay is optional.
        std::optional<size_t> fanMissingErrorDelay;
        if (fan.contains("fan_missing_error_delay"))
        {
            fanMissingErrorDelay =
                fan.at("fan_missing_error_delay").get<size_t>();
        }

        // Handle optional conditions
        auto cond = std::optional<Condition>();
        if (fan.contains("condition"))
        {
            if (!fan["condition"].contains("name"))
            {
                // Log error on missing required parameter
                lg2::error(
                    "Missing required fan monitor condition parameter 'name'");
                throw std::runtime_error(
                    "Missing required fan monitor condition parameter");
            }
            auto name = fan["condition"]["name"].get<std::string>();
            // The function for fan monitoring condition
            // (Must have a supported function within the condition namespace)
            std::transform(name.begin(), name.end(), name.begin(), tolower);
            auto handler = conditions.find(name);
            if (handler != conditions.end())
            {
                cond = handler->second(fan["condition"]);
            }
            else
            {
                lg2::info(
                    "No handler found for configured condition {CONDITION_NAME}",
                    "CONDITION_NAME", name, "JSON_DUMP",
                    fan["condition"].dump());
            }
        }

        // if the fan should be set to functional when plugged in
        bool setFuncOnPresent = false;
        if (fan.contains("set_func_on_present"))
        {
            setFuncOnPresent = fan["set_func_on_present"].get<bool>();
        }

        FanDefinition def{
            .name = fan["inventory"].get<std::string>(),
            .method = method,
            .funcDelay = funcDelay,
            .timeout = timeout,
            .deviation = deviation,
            .upperDeviation = upperDeviation,
            .numSensorFailsForNonfunc = nonfuncSensorsCount,
            .monitorStartDelay = monitorDelay,
            .countInterval = countInterval,
            .nonfuncRotorErrDelay = nonfuncRotorErrorDelay,
            .fanMissingErrDelay = fanMissingErrorDelay,
            .sensorList = std::move(sensorDefs),
            .condition = cond,
            .funcOnPresent = setFuncOnPresent};

        fanDefs.push_back(std::move(def));
    }

    return fanDefs;
}

PowerRuleState getPowerOffPowerRuleState(const json& powerOffConfig)
{
    // The state is optional and defaults to runtime
    PowerRuleState ruleState{PowerRuleState::runtime};

    if (powerOffConfig.contains("state"))
    {
        auto state = powerOffConfig.at("state").get<std::string>();
        if (state == "at_pgood")
        {
            ruleState = PowerRuleState::atPgood;
        }
        else if (state != "runtime")
        {
            lg2::error("Invalid power off state entry {STATE}", "STATE", state);
            throw std::runtime_error("Invalid power off state entry");
        }
    }

    return ruleState;
}

std::unique_ptr<PowerOffCause> getPowerOffCause(const json& powerOffConfig)
{
    std::unique_ptr<PowerOffCause> cause;

    if (!powerOffConfig.contains("count") || !powerOffConfig.contains("cause"))
    {
        lg2::error("Missing 'count' or 'cause' entries in power off config");
        throw std::runtime_error(
            "Missing 'count' or 'cause' entries in power off config");
    }

    auto count = powerOffConfig.at("count").get<size_t>();
    auto powerOffCause = powerOffConfig.at("cause").get<std::string>();

    const std::map<std::string, std::function<std::unique_ptr<PowerOffCause>()>>
        causes{
            {"missing_fan_frus",
             [count]() { return std::make_unique<MissingFanFRUCause>(count); }},
            {"nonfunc_fan_rotors",
             [count]() {
                 return std::make_unique<NonfuncFanRotorCause>(count);
             }},
            {"fan_frus_with_nonfunc_rotors", [count]() {
                 return std::make_unique<FanFRUsWithNonfuncRotorsCause>(count);
             }}};

    auto it = causes.find(powerOffCause);
    if (it != causes.end())
    {
        cause = it->second();
    }
    else
    {
        lg2::error(
            "Invalid power off cause {POWER_OFF_CAUSE} in power off config JSON",
            "POWER_OFF_CAUSE", powerOffCause);
        throw std::runtime_error(
            "Invalid power off cause in power off config JSON");
    }

    return cause;
}

std::unique_ptr<PowerOffAction> getPowerOffAction(
    const json& powerOffConfig,
    std::shared_ptr<PowerInterfaceBase>& powerInterface,
    PowerOffAction::PrePowerOffFunc& func)
{
    std::unique_ptr<PowerOffAction> action;
    if (!powerOffConfig.contains("type"))
    {
        lg2::error("Missing 'type' entry in power off config");
        throw std::runtime_error("Missing 'type' entry in power off config");
    }

    auto type = powerOffConfig.at("type").get<std::string>();

    if (((type == "hard") || (type == "soft")) &&
        !powerOffConfig.contains("delay"))
    {
        lg2::error("Missing 'delay' entry in power off config");
        throw std::runtime_error("Missing 'delay' entry in power off config");
    }
    else if ((type == "epow") &&
             (!powerOffConfig.contains("service_mode_delay") ||
              !powerOffConfig.contains("meltdown_delay")))
    {
        lg2::error(
            "Missing 'service_mode_delay' or 'meltdown_delay' entry in power off config");
        throw std::runtime_error(
            "Missing 'service_mode_delay' or 'meltdown_delay' entry in power off config");
    }

    if (type == "hard")
    {
        action = std::make_unique<HardPowerOff>(
            powerOffConfig.at("delay").get<uint32_t>(), powerInterface, func);
    }
    else if (type == "soft")
    {
        action = std::make_unique<SoftPowerOff>(
            powerOffConfig.at("delay").get<uint32_t>(), powerInterface, func);
    }
    else if (type == "epow")
    {
        action = std::make_unique<EpowPowerOff>(
            powerOffConfig.at("service_mode_delay").get<uint32_t>(),
            powerOffConfig.at("meltdown_delay").get<uint32_t>(), powerInterface,
            func);
    }
    else
    {
        lg2::error("Invalid 'type' entry {TYPE} in power off config", "TYPE",
                   type);
        throw std::runtime_error("Invalid 'type' entry in power off config");
    }

    return action;
}

std::vector<std::unique_ptr<PowerOffRule>> getPowerOffRules(
    const json& obj, std::shared_ptr<PowerInterfaceBase>& powerInterface,
    PowerOffAction::PrePowerOffFunc& func)
{
    std::vector<std::unique_ptr<PowerOffRule>> rules;

    if (!(obj.contains("fault_handling") &&
          obj.at("fault_handling").contains("power_off_config")))
    {
        return rules;
    }

    for (const auto& config : obj.at("fault_handling").at("power_off_config"))
    {
        auto state = getPowerOffPowerRuleState(config);
        auto cause = getPowerOffCause(config);
        auto action = getPowerOffAction(config, powerInterface, func);

        auto rule = std::make_unique<PowerOffRule>(
            std::move(state), std::move(cause), std::move(action));
        rules.push_back(std::move(rule));
    }

    return rules;
}

std::optional<size_t> getNumNonfuncRotorsBeforeError(const json& obj)
{
    std::optional<size_t> num;

    if (obj.contains("fault_handling"))
    {
        // Defaults to 1 if not present inside of 'fault_handling'.
        num = obj.at("fault_handling")
                  .value("num_nonfunc_rotors_before_error", 1);
    }

    return num;
}

} // namespace phosphor::fan::monitor
