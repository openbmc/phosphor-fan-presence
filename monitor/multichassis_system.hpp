// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include "../dbus_paths.hpp"
#include "chassis.hpp"
#include "multichassis_types.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>

#include <memory>
#include <vector>

namespace phosphor::fan::monitor::multi_chassis
{
class MultiChassisSystem
{
  public:
    MultiChassisSystem(const MultiChassisSystem&) = delete;
    MultiChassisSystem(MultiChassisSystem&&) = delete;
    MultiChassisSystem& operator=(const MultiChassisSystem&) = delete;
    MultiChassisSystem& operator=(MultiChassisSystem&&) = delete;
    ~MultiChassisSystem() = default;

    /**
     * Constructor
     *
     * @param[in] mode - Mode of the fan monitor
     * @param[in] bus - Sdbusplus bus object
     * @param[in] event - Event loop reference
     *
     */
    MultiChassisSystem(Mode mode, sdbusplus::bus_t& bus,
                       const sdeventplus::Event& event);

    /**
     * @brief Initiate the chassis objects in the system
     *
     * @param[in] chassisDefs - Vector of ChassisDefinition objects for the
     * chassis
     * @param[in] fanTypeDefs - Vector of FanTypeDefinition objects for the
     * different fan types in the system
     */
    void initChassis(const std::vector<ChassisDefinition> chassisDefs,
                     const std::vector<FanTypeDefinition> fanTypeDefs);

    /**
     * @brief Start the system by parsing the JSON object and initiating the
     * chassis objects
     */
    void start();

    /**
     * @brief Callback function to handle receiving a HUP signal to reload the
     * JSON configuration.
     */
    void sighupHandler(sdeventplus::source::Signal&,
                       const struct signalfd_siginfo*);

    /**
     * @brief Callback function to handle receiving a USR1 signal to dump
     * debug data to a file.
     */
    void dumpDebugData(sdeventplus::source::Signal&,
                       const struct signalfd_siginfo*);

    /**
     * @brief Getter for chassis vector. Returns a vector of unique
     * pointers to chassis objects.
     */
    const std::vector<std::unique_ptr<Chassis>>& getChassis() const
    {
        return _chassis;
    }

  private:
    /* The mode of fan monitor */
    Mode _mode;

    /* The sdbusplus bus object */
    sdbusplus::bus_t& _bus;

    /* The event loop reference */
    const sdeventplus::Event& _event;

    /* Vector of chassis objects */
    std::vector<std::unique_ptr<Chassis>> _chassis;

    /**
     * @brief The thermal alert D-Bus object
     */
    ThermalAlertObject _thermalAlert;

    /**
     * @brief The name of the dump file
     */
    static const std::string dumpFile;

    /**
     * @brief True if config file has been loaded
     */
    bool _loaded = false;

    /**
     * @brief Captures tach sensor data as JSON for use in
     *        fan fault and fan missing event logs.
     *
     * @return json - The JSON data
     */
    json captureSensorData();
};
} // namespace phosphor::fan::monitor::multi_chassis
