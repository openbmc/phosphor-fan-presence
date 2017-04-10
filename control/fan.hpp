#pragma once
#include <sdbusplus/bus.hpp>
#include "types.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{


/**
 * @class Represents a fan enclosure
 */
class Fan
{
    public:

        Fan() = delete;
        Fan(const Fan&) = delete;
        Fan(Fan&&) = default;
        Fan& operator=(const Fan&) = delete;
        Fan& operator=(Fan&&) = delete;
        ~Fan() = default;

        /**
         * Creates a fan object with sensors specified by
         * the fan definition data.
         *
         * @param[in] bus - the dbus object
         * @param[in] def - the fan definition data
         */
        Fan(sdbusplus::bus::bus& bus, const FanDefinition& def);

        /**
         * Sets the speed value on all contained sensors
         *
         * @param[in] speed - the value to set
         */
        void setSpeed(uint64_t speed);

    private:

        /**
         * Returns the service name to use for interacting
         * with the fan sensor passed in.
         *
         * @param[in] sensor - the fan tach sensor name
         * @return - the service name
         */
        auto getService(const std::string& sensor);

        /**
         * The dbus object
         */
        sdbusplus::bus::bus& _bus;

        /**
         * The inventory name of the fan
         */
        std::string _name;

        /**
         * Vector of hwmon sensors for the rotors
         */
        std::vector<std::string> _sensors;
};


}
}
}
