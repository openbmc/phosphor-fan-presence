// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "zone.hpp"

#include "hwmon_ffdc.hpp"
#include "json_parser.hpp"
#include "logging.hpp"
#include "multichassis_json_parser.hpp"

namespace phosphor::fan::monitor::multi_chassis
{
using Severity = sdbusplus::xyz::openbmc_project::Logging::server::Entry::Level;

Zone::Zone(const ZoneDefinition& zoneConfig,
           const std::vector<FanTypeDefinition>& fanDefs, sdbusplus::bus_t& bus,
           Mode mode, const sdeventplus::Event& event,
           ThermalAlertObject& thermalAlert, PowerState& powerState) :
    _zoneConfig(zoneConfig), _fanDefs(fanDefs), _name(zoneConfig.name),
    _mode(mode), _bus(bus), _event(event), _thermalAlert(thermalAlert),
    _powerState(powerState)
{
    namespace match = sdbusplus::bus::match;

    // must be done before service detection
    _inventoryMatch = std::make_unique<sdbusplus::bus::match_t>(
        _bus, match::rules::nameOwnerChanged(util::INVENTORY_SVC),
        std::bind(&Zone::inventoryOnlineCb, this, std::placeholders::_1));

    bool invServiceRunning = util::SDBusPlus::callMethodAndRead<bool>(
        _bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
        "org.freedesktop.DBus", "NameHasOwner", util::INVENTORY_SVC);

    if (invServiceRunning)
    {
        _inventoryMatch.reset();

        if (!_loaded)
        {
            init(zoneConfig, fanDefs);
            for (const auto& fan : _fans)
            {
                fan->init();
            }
        }
    }
    lg2::info("Zone object {ZONE} created", "ZONE", _name);
}

void Zone::logShutdownError()
{
    if (_lastError)
    {
        getLogger().log("Re-committing previous fan error before power off");

        // Still use the latest sensor data
        auto sensorData = captureSensorData();
        _lastError->commit(sensorData, true);
    }
}

void Zone::fanStatusChange(const Fan& fan, bool skipRulesCheck)
{
    updateFanHealth(fan);

    if (_powerState.isPowerOn() && !skipRulesCheck)
    {
        std::for_each(_powerOffRules.begin(), _powerOffRules.end(),
                      [this](auto& rule) {
                          rule->check(PowerRuleState::runtime, _fanHealth);
                      });
    }
}

void Zone::fanMissingErrorTimerExpired(const Fan& fan)
{
    std::string fanPath{util::INVENTORY_PATH + fan.getName()};

    getLogger().log(
        std::format("Creating event log for missing fan {}", fanPath),
        Logger::error);

    auto error = std::make_unique<FanError>(
        "xyz.openbmc_project.Fan.Error.Missing", fanPath, "", Severity::Error);

    auto sensorData = captureSensorData();
    error->commit(sensorData);

    // Save the error so it can be committed again on a power off.
    _lastError = std::move(error);
}

void Zone::sensorErrorTimerExpired(const Fan& fan, const TachSensor& sensor)
{
    std::string fanPath{util::INVENTORY_PATH + fan.getName()};

    getLogger().log(
        std::format("Creating event log for faulted fan {} sensor {}", fanPath,
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

json Zone::captureSensorData()
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

void Zone::subscribeSensorsToServices()
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
                        std::format("Fan sensor entry {} not found in D-Bus",
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
                std::bind(&Zone::tachSignalOffline, this, std::placeholders::_1,
                          sensorMap)));
        }
    }
    catch (const util::DBusError&)
    {
        // catch exception from getSubTreeRaw() when fan sensor paths don't
        // exist yet
    }
}

void Zone::updateFanHealth(const Fan& fan)
{
    std::vector<bool> sensorStatus;
    for (const auto& sensor : fan.sensors())
    {
        sensorStatus.push_back(sensor->functional());
    }

    _fanHealth[fan.getName()] =
        std::make_tuple(fan.present(), std::move(sensorStatus));
}

void Zone::tachSignalOffline(sdbusplus::message_t& msg,
                             const SensorMapType& sensorMap)
{
    std::string serviceName, oldOwner, newOwner;

    msg.read(serviceName);
    msg.read(oldOwner);
    msg.read(newOwner);

    // true if sensor server came back online, false -> went offline
    bool hasOwner = !newOwner.empty() && oldOwner.empty();

    std::string stateStr(hasOwner ? "online" : "offline");
    getLogger().log(std::format("Changing sensors for service {} to {}",
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

void Zone::powerStateChanged(bool powerStateOn)
{
    std::for_each(_fans.begin(), _fans.end(), [powerStateOn](auto& fan) {
        fan->powerStateChanged(powerStateOn);
    });

    if (powerStateOn)
    {
        if (!_loaded)
        {
            lg2::error("No conf file found at power on");
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
                      [](auto& rule) { rule->cancel(); });
    }
}

void Zone::setFaultConfig(const json& jsonObj)
{
#ifdef MONITOR_USE_JSON
    std::shared_ptr<PowerInterfaceBase> powerInterface =
        std::make_shared<PowerInterface>(_thermalAlert);

    PowerOffAction::PrePowerOffFunc func =
        std::bind(&Zone::logShutdownError, this);

    _powerOffRules = getPowerOffRules(jsonObj, powerInterface, func);

    _numNonfuncSensorsBeforeError = getNumNonfuncRotorsBeforeError(jsonObj);
#endif
}

void Zone::handleOfflineFanController()
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

void Zone::inventoryOnlineCb(sdbusplus::message_t& msg)
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
        init(_zoneConfig, _fanDefs);
        for (const auto& fan : _fans)
        {
            fan->init();
        }
    }

    // cancel any further notifications about the service state
    _inventoryMatch.reset();
}

void Zone::createBmcDump() const
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
            std::format("Caught exception while creating BMC dump: {}",
                        e.what()),
            Logger::error);
    }
}

void Zone::init(const ZoneDefinition& zoneConfig,
                const std::vector<FanTypeDefinition>& fanDefs)
{
    // Use an empty trust manager since don't need to support trust groups for
    // multi-chassis
    _trust =
        std::make_unique<trust::Manager>(std::vector<CreateGroupFunction>());
    // Clear/set configured fan definitions
    _fans.clear();
    _fanHealth.clear();
    // Retrieve fan definitions and create fan objects to be monitored
    setFans(zoneConfig, fanDefs);
    // set fault config for power off rules
    setFaultConfig(zoneConfig.faultHandling);
    lg2::info("Configuration loaded for Zone {ZONE_NAME}", "ZONE_NAME", _name);

    _loaded = true;

    if (_powerState.isPowerOn())
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

FanDefinition Zone::getFullDefFromType(const FanTypeDefinition& fanType,
                                       const FanAssignment& fanAssign)
{
    if (fanType.type != fanAssign.type)
    {
        lg2::error(
            "Mismatched type for getting full fan definition from fan type definition and fan assignment");
        throw std::runtime_error(
            "Mismatched type for getting full fan definition from fan type definition and fan assignment");
    }
    std::vector<phosphor::fan::monitor::SensorDefinition>
        fullSensorList; // list of sensor definitions (single-chassis form) for
                        // the full fan definition
    for (const auto& sensor : fanType.sensorList)
    {
        fullSensorList.push_back(phosphor::fan::monitor::SensorDefinition{
            .name = fanAssign.shortName + sensor.pathSuffix,
            .hasTarget = sensor.hasTarget,
            .targetInterface = sensor.targetInterface,
            .targetPath = sensor.targetPath,
            .factor = sensor.factor,
            .offset = sensor.offset,
            .threshold = sensor.threshold,
            .ignoreAboveMax = sensor.ignoreAboveMax});
    }
    return FanDefinition{
        .name = fanAssign.inventoryBase + fanAssign.shortName,
        .method = fanType.method,
        .funcDelay = fanType.funcDelay,
        .timeout = fanType.timeout,
        .deviation = fanType.deviation,
        .upperDeviation = fanType.upperDeviation,
        .numSensorFailsForNonfunc = fanType.numSensorFailsForNonfunc,
        .monitorStartDelay = fanType.monitorStartDelay,
        .countInterval = fanType.countInterval,
        .nonfuncRotorErrDelay = fanType.nonfuncRotorErrDelay,
        .fanMissingErrDelay = fanType.fanMissingErrDelay,
        .sensorList = std::move(fullSensorList),
        .condition = fanType.condition,
        .funcOnPresent = fanType.funcOnPresent};
}

void Zone::setFans(const ZoneDefinition& zoneConfig,
                   const std::vector<FanTypeDefinition>& fanDefs)
{
    for (const auto& fan : zoneConfig.fans)
    {
        // search the fan type definitions for the matching fan type to the fan
        // in the zone config
        auto fanTypeConfig =
            std::find_if(fanDefs.begin(), fanDefs.end(),
                         [&fan](const FanTypeDefinition& fanTypeDef) {
                             return fan.type == fanTypeDef.type;
                         });
        if (fanTypeConfig == fanDefs.end())
        {
            // fan type not found in the fan_type_definitions part of the JSON
            lg2::error("Fan type '{TYPE}' not found in fan type definitions",
                       "TYPE", fan.type);
            throw std::runtime_error(
                "Fan type not found in fan type definitions");
        }

        // Check if a condition exists on the fan
        auto condition = fanTypeConfig->condition;
        if (condition)
        {
            // Condition exists, skip adding fan if it fails
            if (!(*condition)(_bus))
            {
                continue;
            }
        }

        _fans.emplace_back(std::make_unique<Fan>(
            _mode, _bus, _event, _trust,
            getFullDefFromType(*fanTypeConfig, fan), *this));

        updateFanHealth(*(_fans.back()));
    }
}
} // namespace phosphor::fan::monitor::multi_chassis
