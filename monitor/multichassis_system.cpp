// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "multichassis_system.hpp"

namespace phosphor::fan::monitor::multi_chassis
{
const std::string MultiChassisSystem::dumpFile = "/tmp/fan_monitor_dump.json";

MultiChassisSystem::MultiChassisSystem(Mode mode, sdbusplus::bus_t& bus,
                                       const sdeventplus::Event& event) :
    _mode(mode), _bus(bus), _event(event),
    _thermalAlert(bus, THERMAL_ALERT_OBJPATH)
{}

void MultiChassisSystem::initChassis(std::vector<ChassisDefinition> chassisDefs,
                                     std::vector<FanTypeDefinition> fanTypeDefs)
{
    _chassis.clear();
    for (const auto& chassisDef : chassisDefs)
    {
        // instantiate a chassis object for each chassis number in a chassis
        // definition
        for (int chassisNum : chassisDef.chassisNumbers)
        {
            _chassis.emplace_back(std::make_unique<Chassis>(
                chassisDef, fanTypeDefs, _bus, _mode, _event, _thermalAlert,
                std::to_string(chassisNum)));
        }
    }
}

void MultiChassisSystem::start()
{
    // get json object, parse, and call initChassis
    json jsonObj = getJsonObj();
    if (!jsonObj.contains("fan_type_definitions"))
    {
        lg2::error(
            "Missing required 'fan_type_definitions' list in config file.");
        throw std::runtime_error(
            "Missing required 'fan_type_definitions' list in config file.");
    }
    if (!jsonObj.contains("chassis_definitions"))
    {
        lg2::error(
            "Missing required 'chassis_definitions' list in config file.");
        throw std::runtime_error(
            "Missing required 'chassis_definitions' list in config file.");
    }
    std::vector<FanTypeDefinition> fanTypeDefs =
        getFanDefs(jsonObj["fan_type_definitions"]);
    std::vector<ChassisDefinition> chassisDefs =
        getChassisDefs(jsonObj["chassis_definitions"]);
    initChassis(chassisDefs, fanTypeDefs);
    _loaded = true;
}

void MultiChassisSystem::sighupHandler(sdeventplus::source::Signal&,
                                       const struct signalfd_siginfo*)
{
    try
    {
        start();
    }
    catch (std::runtime_error& re)
    {
        lg2::error(
            "Error reloading config, no config changes made: {LOAD_ERROR}",
            "LOAD_ERROR", re);
    }
}

void MultiChassisSystem::dumpDebugData(sdeventplus::source::Signal&,
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

    std::ofstream file{MultiChassisSystem::dumpFile};
    if (!file)
    {
        lg2::error("Could not open file for fan monitor dump");
    }
    else
    {
        file << std::setw(4) << output;
    }
}

json MultiChassisSystem::captureSensorData()
{
    json data;

    // go through all chassis, zones, fans, and sensors to retrieve sensor data
    for (const auto& chassis : _chassis)
    {
        for (const auto& zone : chassis->getZones())
        {
            for (const auto& fan : zone->getFans())
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
                        values["prev_targets"] =
                            json(sensor->getPrevTarget()).dump();
                    }

                    if (sensor->getMethod() == MethodMode::count)
                    {
                        values["ticks"] = sensor->getCounter();
                    }
                    data["sensors"][sensor->name()] = values;
                }
            }
        }
    }
    return data;
}

} // namespace phosphor::fan::monitor::multi_chassis
