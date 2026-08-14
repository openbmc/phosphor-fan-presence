// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "chassis.hpp"

#include "logging.hpp"

namespace phosphor::fan::monitor::multi_chassis
{
Chassis::Chassis(const ChassisDefinition& chassisConfig,
                 const std::vector<FanTypeDefinition>& fanDefs,
                 sdbusplus::bus_t& bus, const sdeventplus::Event& event,
                 ThermalAlertObject& thermalAlert) :
    _chassisConfig(chassisConfig), _fanDefs(fanDefs), _bus(bus), _event(event),
    _thermalAlert(thermalAlert),
    _powerState(std::make_unique<PGoodState>(
        bus,
        std::bind(&Chassis::powerStateChanged, this, std::placeholders::_1),
        std::to_string(chassisConfig.chassisNum))),
    _chassisName(chassisConfig.name),
    _availPropUsed(chassisConfig.availPropUsed)
{
    if (_availPropUsed)
    {
        _availableMatch = std::make_unique<sdbusplus::match>(
            bus,
            sdbusplus::match_rules::propertiesChanged(
                CHASSIS_PATH_BASE + _chassisName, AVAILABILITY_INTF),
            std::bind(&Chassis::availableChanged, this, std::placeholders::_1));
        _availIfaceAddedMatch = std::make_unique<sdbusplus::match>(
            bus,
            sdbusplus::match_rules::interfacesAddedAtPath(
                CHASSIS_PATH_BASE + _chassisName),
            std::bind(&Chassis::availIfaceAdded, this, std::placeholders::_1));
    }
    // Initialize Present listeners regardless of if Available is used
    _presentMatch = std::make_unique<sdbusplus::match>(
        bus,
        sdbusplus::match_rules::propertiesChanged(
            CHASSIS_PATH_BASE + _chassisName, util::INV_ITEM_IFACE),
        std::bind(&Chassis::presentChanged, this, std::placeholders::_1));
    _invItemIfaceAddedMatch = std::make_unique<sdbusplus::match>(
        bus,
        sdbusplus::match_rules::interfacesAddedAtPath(
            CHASSIS_PATH_BASE + _chassisName),
        std::bind(&Chassis::invItemIfaceAdded, this, std::placeholders::_1));
    init();
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
    try
    {
        _present = util::SDBusPlus::getProperty<bool>(
            _bus, CHASSIS_PATH_BASE + _chassisName, util::INV_ITEM_IFACE,
            "Present");
    }
    catch (const util::DBusServiceError& e)
    {
        // Catch errors for when inventory service isn't running (e.g. automated
        // testing)
    }
    if (!_availPropUsed)
    {
        // If system does not use Available property, check Present and create
        // Zones based on that
        if (_present)
        {
            createZones();
            lg2::info("Chassis {CHASSIS} init complete.", "CHASSIS",
                      _chassisName);
        }
        else
        {
            lg2::info(
                "Chassis {CHASSIS} not initiated. Present property is false and Available is not being used",
                "CHASSIS", _chassisName);
        }
        return;
    }
    try
    {
        _available = util::SDBusPlus::getProperty<bool>(
            _bus, CHASSIS_PATH_BASE + _chassisName, AVAILABILITY_INTF,
            "Available");
        if (_available && _present)
        {
            createZones();
            lg2::info("Chassis {CHASSIS} init complete.", "CHASSIS",
                      _chassisName);
        }
        else
        {
            // If Available is used but false, wait for it to become true
            lg2::info(
                "Chassis {CHASSIS} not initiated. Available and/or Present property is false",
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

    // Check Present and Available before creating zones for monitoring
    if (_available && _present)
    {
        createZones();
    }
    else
    {
        lg2::info(
            "Available and/or Present property for chassis {CHASSIS} is false. Stopping monitoring...",
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
        sdbusplus::object_path,
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
    if (!_available || !_present)
    {
        // If Available property is used, don't monitor unless both Present and
        // Available are true
        getLogger().log(std::format(
            "Available interface {} added and chassis {} is not available and/or present.",
            AVAILABILITY_INTF, _chassisName));
        _zones.clear();
        return;
    }
    createZones();
}

void Chassis::createZones()
{
    _zones.clear();
    for (const auto& zoneDef : _chassisConfig.zones)
    {
        _zones.emplace_back(std::make_unique<Zone>(
            zoneDef, _fanDefs, _bus, _event, _thermalAlert, *_powerState));
    }
}

void Chassis::presentChanged(sdbusplus::message_t& msg)
{
    auto [interface, properties] =
        msg.unpack<std::string, std::map<std::string, std::variant<bool>>>();
    if (interface == util::INV_ITEM_IFACE)
    {
        auto presentProp = properties.find("Present");
        if (presentProp != properties.end())
        {
            _present = std::get<bool>(presentProp->second);
        }
    }

    if (_present)
    {
        createZones();
    }
    else
    {
        lg2::info(
            "Present property for chassis {CHASSIS} is false. Stopping monitoring...",
            "CHASSIS", _chassisName);
        _zones.clear();
    }
}

void Chassis::invItemIfaceAdded(sdbusplus::message_t& msg)
{
    auto [path, interfaces] = msg.unpack<
        sdbusplus::object_path,
        std::map<std::string, std::map<std::string, std::variant<bool>>>>();

    auto properties = interfaces.find(util::INV_ITEM_IFACE);
    if (properties == interfaces.end())
    {
        return;
    }
    auto property = properties->second.find("Present");
    if (property == properties->second.end())
    {
        return;
    }
    _present = std::get<bool>(property->second);

    if (!_present)
    {
        getLogger().log(std::format(
            "Inventory item interface {} added for chassis {} but Present property on interface false.",
            util::INV_ITEM_IFACE, _chassisName));
        _zones.clear();
        return;
    }
    createZones();
}
} // namespace phosphor::fan::monitor::multi_chassis
