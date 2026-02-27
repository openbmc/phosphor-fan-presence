// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "multichassis_json_parser.hpp"

#include "conditions.hpp"
#include "json_config.hpp"
#include "system.hpp"
#include "tach_sensor.hpp"
#include "types.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/lg2.hpp>

#include <algorithm>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace phosphor::fan::monitor::multi_chassis
{
const std::map<std::string, condHandler> conditions = {
    {"propertiesmatch", monitor::condition::getPropertiesMatch}};
const std::map<std::string, size_t> methods = {
    {"timebased", MethodMode::timebased}, {"count", MethodMode::count}};
const std::map<std::string, std::string> _jsonVars{
    {"chassisHolder", "${chassis}"}};

const std::vector<SensorDefinition> getSensorDefs(const json& sensors)
{
    std::vector<SensorDefinition> sensorDefs;

    for (const auto& sensor : sensors)
    {
        if (!sensor.contains("path_suffix") || !sensor.contains("has_target"))
        {
            // Log error on missing required parameters
            lg2::error(
                "Missing required fan sensor definition parameters 'path_suffix, has_target'");
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
            .pathSuffix = sensor["path_suffix"].get<std::string>(),
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
const std::vector<FanTypeDefinition> getFanDefs(const json& fans)
{
    std::vector<FanTypeDefinition> fanDefs;

    for (const auto& fan : fans)
    {
        if (!fan.contains("type") || !fan.contains("deviation") ||
            !fan.contains("sensors"))
        {
            lg2::error(
                "Missing one of required parameters 'type, deviation, sensors'");
            throw std::runtime_error(
                "Missing required fan type definition parameter");
        }

        // Valid deviaiton range is 0 - 100%
        auto deviation = fan["deviation"].get<size_t>();
        if (deviation > 100)
        {
            lg2::error(
                "Invalid deviation of {DEVIATION} found, must be between 0 and 100",
                "DEVIATION", deviation);
            throw std::runtime_error(
                "Invalid deviation found, must be between 0 and 100");
        }

        // Upper deviation defaults to the deviation value, but can be
        // separately specified
        size_t upperDeviation = deviation;
        if (fan.contains("upper_deviation"))
        {
            upperDeviation = fan["upper_deviation"].get<size_t>();
            if (upperDeviation > 100)
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

        // nonfunc_rotor_error_delay is optional and defaults to 0
        size_t nonfuncRotorErrorDelay = 0;
        if (fan.contains("nonfunc_rotor_error_delay"))
        {
            nonfuncRotorErrorDelay =
                fan["nonfunc_rotor_error_delay"].get<size_t>();
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

        FanTypeDefinition def{
            .type = fan["type"].get<std::string>(),
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

const std::vector<ChassisDefinition> getChassisDefs(const json& chassis)
{
    std::vector<ChassisDefinition> chassisDefs;
    for (const auto& chassisConfig : chassis)
    {
        if (!chassisConfig.contains("chassis_numbers") ||
            !chassisConfig.contains("zones") || !chassisConfig.contains("name"))
        {
            lg2::error(
                "Missing one of required fields 'chassis_numbers, zones'");
            throw std::runtime_error("Missing required field");
        }
        std::vector<int> chassisNums =
            chassisConfig["chassis_numbers"].get<std::vector<int>>();
        if (!std::ranges::all_of(chassisNums, [](int num) { return num >= 0; }))
        {
            // Throw error for negative chassis number
            throw std::runtime_error(
                "Chassis numbers must be greater than or equal to 0");
        }
        // create a new ChassisDefinition object for each chassis number
        for (int chassisNum : chassisNums)
        {
            std::string name = chassisConfig["name"].get<std::string>();
            // Expand all variables in name
            for (const auto& [varName, varHolder] : _jsonVars)
            {
                if (varName == "chassisHolder")
                {
                    expandVar("chassisHolder", name,
                              std::to_string(chassisNum));
                }
            }
            std::vector<ZoneDefinition> zoneDefs =
                getZoneDefs(chassisConfig["zones"], chassisNum);
            ChassisDefinition def{
                .name = name,
                .chassisNumbers =
                    chassisConfig["chassis_numbers"].get<std::vector<int>>(),
                .zones = std::move(zoneDefs),
                .availPropUsed = chassisConfig.value("availPropUsed", false),
                .chassisNumRef = chassisNum};
            chassisDefs.push_back(std::move(def));
        }
    }
    return chassisDefs;
}

const std::vector<ZoneDefinition> getZoneDefs(const json& zones, int chassisNum)
{
    std::vector<ZoneDefinition> zoneDefs;
    for (const auto& zoneConfig : zones)
    {
        if (!zoneConfig.contains("name") || !zoneConfig.contains("fans") ||
            !zoneConfig.contains("fault_handling"))
        {
            lg2::error(
                "Missing one of required fields 'name, fans, fault_handling'");
            throw std::runtime_error("Missing required field");
        }
        std::string name = zoneConfig["name"].get<std::string>();
        // Expand all variables in name
        for (const auto& [varName, varHolder] : _jsonVars)
        {
            if (varName == "chassisHolder")
            {
                expandVar("chassisHolder", name, std::to_string(chassisNum));
            }
        }
        std::vector<FanAssignment> fanAssigns =
            getFanAssigns(zoneConfig["fans"], chassisNum);
        ZoneDefinition def{
            .name = name,
            .fans = std::move(fanAssigns),
            .faultHandling =
                json{{"fault_handling", zoneConfig["fault_handling"]}}};
        zoneDefs.push_back(def);
    }
    return zoneDefs;
}

const std::vector<FanAssignment> getFanAssigns(const json& fanList,
                                               int chassisNum)
{
    std::vector<FanAssignment> fanAssigns;
    for (const auto& fan : fanList)
    {
        if (!fan.contains("type") || !fan.contains("inventory_base") ||
            !fan.contains("short_name"))
        {
            lg2::error(
                "Missing one of required fields 'type, inventory_base, short_name' in fan object");
            throw std::runtime_error("Missing required field in fan object");
        }
        std::string inventoryBase = fan["inventory_base"].get<std::string>();
        std::string shortName = fan["short_name"].get<std::string>();
        // expand all variables in inventoryBase and shortName
        for (const auto& [varName, varHolder] : _jsonVars)
        {
            if (varName == "chassisHolder")
            {
                expandVar("chassisHolder", inventoryBase,
                          std::to_string(chassisNum));
                expandVar("chassisHolder", shortName,
                          std::to_string(chassisNum));
            }
        }
        FanAssignment def{.type = fan["type"].get<std::string>(),
                          .inventoryBase = inventoryBase,
                          .shortName = shortName};
        fanAssigns.push_back(def);
    }
    return fanAssigns;
}

void expandVar(const std::string& varName, std::string& varString,
               const std::string& val)
{
    auto varPair = _jsonVars.find(varName);
    if (varPair != _jsonVars.end())
    {
        auto varHolder = varPair->second;
        if (varString.contains(varHolder))
        {
            // replace all instances of the variable in a string with the value
            for (size_t varPlace = varString.find(varHolder);
                 varPlace != std::string::npos;
                 varPlace = varString.find(varHolder, varPlace + val.length()))
            {
                varString.replace(varPlace, varHolder.length(), val);
            }
        }
    }
}

} // namespace phosphor::fan::monitor::multi_chassis
