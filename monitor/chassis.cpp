// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "chassis.hpp"

#include "logging.hpp"

namespace phosphor::fan::monitor::multi_chassis
{
Chassis::Chassis(const ChassisDefinition& chassisConfig,
                 const std::vector<FanTypeDefinition>& fanDefs,
                 sdbusplus::bus_t& bus, Mode mode,
                 const sdeventplus::Event& event,
                 ThermalAlertObject& thermalAlert) :
    _chassisConfig(chassisConfig), _fanDefs(fanDefs), _bus(bus), _mode(mode),
    _event(event), _thermalAlert(thermalAlert),
#ifdef MONITOR_USE_HOST_STATE
    _powerState(std::make_unique<HostPowerState>(
#else
    _powerState(std::make_unique<PGoodState>(
#endif
        bus,
        std::bind(&Chassis::powerStateChanged, this, std::placeholders::_1),
        std::to_string(chassisConfig.chassisNumRef))),
    _chassisName(chassisConfig.name),
    _availPropUsed(chassisConfig.availPropUsed)
{
    if (_availPropUsed)
    {
        _availableMatch = std::make_unique<sdbusplus::bus::match_t>(
            bus,
            sdbusplus::bus::match::rules::propertiesChanged(
                CHASSIS_PATH_BASE + _chassisName, AVAILABILITY_INTF),
            std::bind(&Chassis::availableChanged, this, std::placeholders::_1));
        _availIfaceAddedMatch = std::make_unique<sdbusplus::bus::match_t>(
            bus,
            sdbusplus::bus::match::rules::interfacesAddedAtPath(
                CHASSIS_PATH_BASE + _chassisName),
            std::bind(&Chassis::availIfaceAdded, this, std::placeholders::_1));
    }
    init();
    lg2::info("Chassis object {CHASSIS} created", "CHASSIS", _chassisName);
}

void Chassis::powerStateChanged(bool powerStateOn)
{
    for (const auto& zone : _zones)
    {
        zone->powerStateChanged(powerStateOn);
    }
}

void Chassis::init()
{
    lg2::info("Chassis {CHASSIS} init starting...", "CHASSIS", _chassisName);
    if (!_availPropUsed)
    {
        // If system does not use Available property just create the zones
        _zones.clear();
        for (const auto& zoneDef : _chassisConfig.zones)
        {
            _zones.emplace_back(
                std::make_unique<Zone>(zoneDef, _fanDefs, _bus, _mode, _event,
                                       _thermalAlert, *_powerState));
        }
        lg2::info("Chassis {CHASSIS} init complete.", "CHASSIS", _chassisName);
        return;
    }
    try
    {
        _available = util::SDBusPlus::getProperty<bool>(
            _bus, CHASSIS_PATH_BASE + _chassisName, AVAILABILITY_INTF,
            "Available");
        if (_available)
        {
            _zones.clear();
            for (const auto& zoneDef : _chassisConfig.zones)
            {
                _zones.emplace_back(std::make_unique<Zone>(
                    zoneDef, _fanDefs, _bus, _mode, _event, _thermalAlert,
                    *_powerState));
            }
            lg2::info("Chassis {CHASSIS} init complete.", "CHASSIS",
                      _chassisName);
        }
        else
        {
            // If Available is used but false, wait for it to become true
            lg2::info(
                "Chassis {CHASSIS} not initiated. Available property is false",
                "CHASSIS", _chassisName);
        }
    }
    catch (const util::DBusServiceError& e)
    {
        // Wait for propertiesChanged or interfacesAddedAtPath signal for
        // Available property
    }
}

void Chassis::availableChanged(sdbusplus::message_t& msg)
{
    if (!_availPropUsed)
    {
        // If system does not use Available property, do nothing
        return;
    }

    auto [interface, properties] =
        msg.unpack<std::string, std::map<std::string, std::variant<bool>>>();
    if (interface == AVAILABILITY_INTF)
    {
        // Handle Availability Property Change
        auto availableProp = properties.find("Available");
        if (availableProp != properties.end())
        {
            _available = std::get<bool>(availableProp->second);
        }
    }

    // Only monitor a zone if the chassis is available, otherwise
    // don't monitor
    if (_available)
    {
        init();
    }
    else
    {
        lg2::info(
            "Available property for chassis {CHASSIS} is false. Stopping monitoring...",
            "CHASSIS", _chassisName);
        _zones.clear();
    }
}

void Chassis::availIfaceAdded(sdbusplus::message_t& msg)
{
    if (!_availPropUsed)
    {
        // If system does not use Available property, do nothing
        return;
    }

    auto [path, interfaces] = msg.unpack<
        sdbusplus::message::object_path,
        std::map<std::string, std::map<std::string, std::variant<bool>>>>();

    auto properties = interfaces.find(AVAILABILITY_INTF);
    if (properties == interfaces.end())
    {
        return;
    }
    auto property = properties->second.find("Available");
    if (property == properties->second.end())
    {
        return;
    }
    _available = std::get<bool>(property->second);
    if (!_available)
    {
        // If system uses Available property and it is not there or false, stop
        // monitoring
        getLogger().log(std::format(
            "Available interface {} added and chassis {} is not available.",
            AVAILABILITY_INTF, _chassisName));
        _zones.clear();
        return;
    }
    init();
}
} // namespace phosphor::fan::monitor::multi_chassis
