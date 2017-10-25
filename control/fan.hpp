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
 * @class Fan
 *
 * Represents a fan.  It has sensors used for setting speeds
 * on all of the contained rotors.  There may or may not be
 * a 1 to 1 correspondence between rotors and sensors, depending
 * on how the hardware and hwmon is configured.
 *
 */
class Fan
{
    public:

        Fan() = delete;
        Fan(const Fan&) = delete;
        Fan(Fan&&) = default;
        Fan& operator=(const Fan&) = delete;
        Fan& operator=(Fan&&) = default;
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

        /**
         * @brief Get the current fan target speed
         *
         * @param[in] fan - fan to get target speed from
         *
         * @return - The target speed of the fan
         */
        inline auto getTargetSpeed() const
        {
            return _targetSpeed;
        }

    private:

        /**
         * Returns the service name to use for interacting
         * with the fan sensor passed in.
         *
         * @param[in] sensor - the fan tach sensor name
         * @return - the service name
         */
        std::string getService(const std::string& sensor);

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

        /**
         * Target speed for this fan
         */
        uint64_t _targetSpeed;
};


}
}
}
