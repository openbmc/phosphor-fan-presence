#pragma once

#include <memory>
#include <vector>
#include <sdbusplus/bus.hpp>
#include "types.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{

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
         */
        Manager(sdbusplus::bus::bus& bus);

    private:

        /**
         * The dbus object
         */
        sdbusplus::bus::bus& _bus;

        /**
         * The fan zone layout for the system.
         * This is generated data.
         */
        static const std::vector<ZoneGroup> _zoneLayouts;
};


}
}
}
