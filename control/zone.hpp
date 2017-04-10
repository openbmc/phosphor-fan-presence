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

        Zone() = delete;
        Zone(const Zone&) = delete;
        Zone(Zone&&) = default;
        Zone& operator=(const Zone&) = delete;
        Zone& operator=(Zone&&) = delete;
        ~Zone() = default;

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
         * Sets the zone to full speed
         */
        inline void setFullSpeed()
        {
            if (_fullSpeed != 0)
            {
                setSpeed(_fullSpeed);
            }
        }

    private:

        /**
         * The dbus object
         */
        sdbusplus::bus::bus& _bus;

        /**
         * Full speed for the zone
         */
        const uint64_t _fullSpeed;

        /**
         * The zone number
         */
        const ssize_t _zoneNum;

        /**
         * The vector of fans in this zone
         */
        std::vector<std::unique_ptr<Fan>> _fans;
};

}
}
}
