#pragma once

#include <sdbusplus/bus.hpp>
#include <tuple>
#include <vector>
#include "tach_sensor.hpp"
#include "types.hpp"

namespace phosphor
{
namespace fan
{
namespace monitor
{


/**
 * @class Fan
 *
 * Represents a fan, which can contain 1 or more sensors which
 * loosely correspond to rotors.  See below.
 *
 * There is a sensor when hwmon exposes one, which means there is a
 * speed value to be read.  Sometimes there is a sensor per rotor,
 * and other times multiple rotors just use 1 sensor total where
 * the sensor reports the slowest speed of all of the rotors.
 *
 * A rotor's speed is set by writing the Target value of a sensor.
 * Sometimes each sensor in a fan supports having a Target, and other
 * times not all of them do.  A TachSensor object knows if it supports
 * the Target property.
 *
 * The strategy for monitoring fan speeds is as follows:
 *
 * Every time a Target (new speed written) or Input (actual speed read)
 * sensor changes, check if the input value is within some range of the target
 * value.  If it isn't, start a timer at the end of which the sensor will be
 * set to not functional.  If enough sensors in the fan are now nonfunctional,
 * set the whole fan to nonfunctional in the inventory.
 *
 * When sensor inputs come back within a specified range of the target,
 * stop its timer if running, make the sensor functional again if it wasn't,
 * and if enough sensors in the fan are now functional set the whole fan
 * back to functional in the inventory.
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
         * @brief Constructor
         *
         * @param bus - the dbus object
         * @param events - pointer to sd_event object
         * @param def - the fan definition structure
         */
        Fan(sdbusplus::bus::bus& bus,
            std::shared_ptr<sd_event>& events,
            const FanDefinition& def);


    private:

        /**
         * @brief Returns the target speed of the sensor
         *
         * If the sensor itself doesn't have a target, it finds
         * the target speed from another sensor.
         *
         * @param[in] sensor - the sensor to get the target speed for
         */
        uint64_t getTargetSpeed(const TachSensor& sensor);

        /**
         * @brief Returns true if the sensor input is not within
         * some deviation of the target.
         *
         * @param[in] sensor - the sensor to check
         */
        bool outOfRange(const TachSensor& sensor);

        /**
         * @brief Returns true if too many sensors are nonfunctional
         *        as defined by _numSensorFailsForNonFunc
         */
        bool tooManySensorsNonfunctional();


        /**
         * @brief the dbus object
         */
        sdbusplus::bus::bus& _bus;

        /**
         * @brief The inventory name of the fan
         */
        const std::string _name;

        /**
         * @brief The percentage that the input speed must be below
         *        the target speed to be considered an error.
         *        Between 0 and 100.
         */
        const size_t _deviation;

        /**
         * The number of sensors that must be nonfunctional at the
         * same time in order for the fan to be set to nonfunctional
         * in the inventory.
         */
        const size_t _numSensorFailsForNonFunc;

        /**
         * @brief The current functional state of the fan
         */
        bool _functional = true;

        /**
         * The sensor objects for the fan
         */
        std::vector<std::unique_ptr<TachSensor>> _sensors;
};

}
}
}
