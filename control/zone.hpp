#pragma once
#include <vector>
#include <sdbusplus/bus.hpp>
#include "fan.hpp"
#include "types.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{

/**
 * @class Represents a fan control zone, which is a group of fans
 * that behave the same.
 */
class Zone
{
    public:

        /**
         * Constructor
         * Creates the appropriate fan objects based on
         * the zone definition data passed in.
         *
         * @param[in] bus - the dbus object
         * @param[in] def - the fan zone definition data
         */
        Zone(sdbusplus::bus::bus& bus,
             const ZoneDefinition& def);

        /**
         * Sets all fans in the zone to the speed
         * passed in
         *
         * @param[in] speed - the fan speed
         */
        void setSpeed(uint64_t speed);

        /**
         * Sets the speed for all fans in the zone when first
         * starting up if the OCC isn't active. A value of
         * 0 for _initialSpeed means don't set any speeds.
         */
        inline void setInitialSpeed()
        {
            if (_initialSpeed != 0)
            {
                setSpeed(_initialSpeed);
            }
        }

    private:

        /**
         * The dbus object
         */
        sdbusplus::bus::bus& _bus;

        /**
         * The speed to set when the app starts up
         */
        const uint64_t _initialSpeed;

        /**
         * The zone number
         */
        const unsigned int _zoneNum;

        /**
         * The vector of fans in this zone
         */
        std::vector<std::unique_ptr<Fan>> _fans;
};

}
}
}
