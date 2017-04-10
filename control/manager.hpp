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
         * Constructor
         * Creates the Zone objects based on the
         * _zoneLayouts data.
         *
         * @param[in] bus - The dbus object
         */
        Manager(sdbusplus::bus::bus& bus);

        /**
         * Sets all fans to their default speed to use
         * on application startup (when the OCC isn't active).
         */
        void setInitialFanSpeed();

    private:

        /**
         * Returns true if all conditions passed in evaluate to true
         *
         * @param TODO
         */
        bool meetsGroupConditions(
            const std::vector<Condition>& conditions);

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
};


}
}
}
