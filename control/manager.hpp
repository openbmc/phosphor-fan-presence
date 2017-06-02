#pragma once

#include <memory>
#include <vector>
#include <sdbusplus/bus.hpp>
#include "types.hpp"
#include "zone.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{

using ZoneMap = std::map<unsigned int,
                         std::unique_ptr<Zone>>;

/**
 * @class Fan control manager
 */
class Manager
{
    public:

        Manager() = delete;
        Manager(const Manager&) = delete;
        Manager(Manager&&) = default;
        Manager& operator=(const Manager&) = delete;
        Manager& operator=(Manager&&) = delete;
        ~Manager() = default;

        /**
         * Constructor
         * Creates the Zone objects based on the
         * _zoneLayouts data.
         *
         * @param[in] bus - The dbus object
         * @param[in] mode - The control mode
         */
        Manager(sdbusplus::bus::bus& bus,
                Mode mode);

        /**
         * Does the fan control inititialization, which is
         * setting fans to full, delaying so they
         * can get there, and starting a target.
         */
        void doInit();

    private:

        /**
         * Starts the obmc-fan-control-ready dbus target
         */
        void startFanControlReadyTarget();

        /**
         * The dbus object
         */
        sdbusplus::bus::bus& _bus;

        /**
         * The fan zones in the system
         */
        ZoneMap _zones;

        /**
         * The fan zone layout for the system.
         * This is generated data.
         */
        static const std::vector<ZoneGroup> _zoneLayouts;

        /**
         * The number of seconds to delay after
         * fans get set to high speed on a power on
         * to give them a chance to get there.
         */
        static const unsigned int _powerOnDelay;

        /**
         * Get the D-Bus service
         *
         * @param[in] bus       - The D-Bus bus object
         * @param[in] path      - The D-Bus path
         * @param[in] interface - The Dbus interface
         * @param[out] service  - The D-Bus service
         *
         */
        static const std::string getService(sdbusplus::bus::bus& bus,
                                            const std::string& path,
                                            const std::string& interface);

        /**
         * Get the current value of the D-Bus property under the specified path
         * and interface.
         *
         * @param[in] bus          - The D-Bus bus object
         * @param[in] path         - The D-Bus path
         * @param[in] interface    - The D-Bus interface
         * @param[in] propertyName - The D-Bus property
         * @param[out] value       - The D-Bus property's value
         */
        template <typename T>
        static void getProperty(sdbusplus::bus::bus& bus,
                                             const std::string& path,
                                             const std::string& interface,
                                             const std::string& propertyName,
                                             T& value);

};


}
}
}
