// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include "logging.hpp"
#include "multichassis_types.hpp"
#include "power_state.hpp"
#include "zone.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/event.hpp>
#include <sdeventplus/source/signal.hpp>

#include <memory>
#include <vector>

namespace phosphor::fan::monitor::multi_chassis
{

// DBus interface for chassis Available property
constexpr auto AVAILABILITY_INTF =
    "xyz.openbmc_project.State.Decorator.Availability";
// Common DBus base path for chassis
constexpr auto CHASSIS_PATH_BASE = "/xyz/openbmc_project/inventory/system/";

class Chassis
{
  public:
    Chassis() = delete;
    Chassis(const Chassis&) = delete;
    Chassis(Chassis&&) = delete;
    Chassis& operator=(const Chassis&) = delete;
    Chassis& operator=(Chassis&&) = delete;
    ~Chassis() = default;

    /**
     * Constructor
     *
     * @param[in] chassisConfig - Chassis configuration object
     * @param[in] fanDefs - Vector of FanTypeDefinition objects representing the
     * different types of fans in the system
     * @param[in] bus - sdbusplus bus object
     * @param[in] mode - Mode of the fan monitor
     * @param[in] event - Event loop reference
     * @param[in] thermalAlert - Reference to ThermalAlertObject
     * @param[in] chassisName - Name of the chassis
     */
    Chassis(const ChassisDefinition& chassisConfig,
            const std::vector<FanTypeDefinition>& fanDefs,
            sdbusplus::bus_t& bus, Mode mode, const sdeventplus::Event& event,
            ThermalAlertObject& thermalAlert, std::string chassisName);

    /**
     * @brief Callback for when the power state changes
     *
     * @param[in] powerStateOn - Flag indicating whether or not power is on
     */
    void powerStateChanged(bool powerStateOn);

    /**
     * @brief Initiate the chassis by creating and assigning zones
     */
    void init();

    /**
     * @brief Callback for when the Present or Available DBus properties change
     *
     * @param[in] msg - DBus message queue containing the interface and
     * properties
     */
    void availableChanged(sdbusplus::message_t& msg);

    /**
     * @brief Returns true if the power is on
     */
    bool isPowerOn() const
    {
        return _powerState->isPowerOn();
    }

    /**
     * @brief Returns the name of the chassis
     */
    inline const std::string getName() const
    {
        return _chassisName;
    }

    /**
     * @brief Returns true if the chassis is available
     */
    bool isAvailable() const
    {
        return _available;
    }

    /**
     * @brief Returns true if the available property is used for this chassis
     */
    bool isAvailPropUsed() const
    {
        return _availPropUsed;
    }

    /**
     * @brief Callback for when the Available interface is posted to DBus
     *
     * @param[in] msg - DBus message queue
     */
    void availIfaceAdded(sdbusplus::message_t& msg);

    /**
     * @brief Getter for zones vector. Returns a vector of unique pointers to
     * zone objects.
     */
    const std::vector<std::unique_ptr<Zone>>& getZones() const
    {
        return _zones;
    }

  private:
    /* Chassis configuration object */
    ChassisDefinition _chassisConfig;

    /* Vector of fan types in the system */
    std::vector<FanTypeDefinition> _fanDefs;

    /* Vector of zones monitored by the chassis */
    std::vector<std::unique_ptr<Zone>> _zones;

    /* The sdbusplus bus object */
    sdbusplus::bus_t& _bus;

    /* The mode of fan monitor */
    Mode _mode;

    /* The event loop reference */
    const sdeventplus::Event& _event;

    /**
     * @brief The thermal alert D-Bus object
     */
    ThermalAlertObject& _thermalAlert;

    /**
     * @brief PowerState object reference
     */
    std::unique_ptr<PowerState> _powerState;

    /**
     * @brief The name of the chassis
     */
    std::string _chassisName;

    /**
     * @brief Match object for the Available property being changed
     */
    std::unique_ptr<sdbusplus::bus::match_t> _availableMatch;

    /**
     * @brief Match object for the Available interface being added
     */
    std::unique_ptr<sdbusplus::bus::match_t> _availIfaceAddedMatch;

    /**
     * @brief The chassis Available status
     */
    bool _available = false;

    /**
     * @brief Denotes whether the Available DBus property is used for this
     * chassis
     */
    bool _availPropUsed = false;
};
} // namespace phosphor::fan::monitor::multi_chassis
