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

        /**
         * The mode the manager will run in:
         *   - init - only set fans to full speed
         *   - control - run normal control algorithms
         */
        enum class Mode
        {
            init,
            control
        };

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
         * @brief Returns the power on delay value, in seconds
         */
        size_t getPowerOnDelay() const
        {
            return _powerOnDelay;
        }

    private:

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
        static const size_t _powerOnDelay;
};


}
}
}
