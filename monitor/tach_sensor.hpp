#pragma once

#include <chrono>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>

namespace phosphor
{
namespace fan
{
namespace monitor
{

class Fan;


/**
 * @class TachSensor
 *
 * This class represents the sensor that reads a tach value.
 * It may also support a Target, which is the property used to
 * set a speed.  Since it doesn't necessarily have a Target, it
 * won't for sure know if it is running too slow, so it leaves
 * that determination to other code.
 *
 * This class has a parent Fan object that knows about all
 * sensors for that fan.
 */
class TachSensor
{
    public:

        TachSensor() = delete;
        TachSensor(const TachSensor&) = delete;
        TachSensor(TachSensor&&) = default;
        TachSensor& operator=(const TachSensor&) = delete;
        TachSensor& operator=(TachSensor&&) = default;
        ~TachSensor() = default;

        /**
         * @brief Constructor
         *
         * @param[in] bus - the dbus object
         * @param[in] fan - the parent fan object
         * @param[in] id - the id of the sensor
         * @param[in] hasTarget - if the sensor supports
         *                        setting the speed
         * @param[in] startupTimeout - First timeout value to use
         * @param[in] timeout - Normal timeout value to use
         */
        TachSensor(sdbusplus::bus::bus& bus,
                   Fan& fan,
                   const std::string& id,
                   bool hasTarget,
                   size_t startupTimeout,
                   size_t timeout);

        /**
         * @brief Returns the target speed value
         */
        inline uint64_t getTarget() const
        {
            return _tachTarget;
        }

        /**
         * @brief Returns the input speed value
         */
        inline int64_t getInput() const
        {
            return _tachInput;
        }

        /**
         * @brief Returns true if sensor has a target
         */
        inline bool hasTarget() const
        {
            return _hasTarget;
        }

        /**
         * Returns true if the hardware behind this
         * sensor is considered working OK/functional.
         */
        inline bool functional() const
        {
            return _functional;
        }

        /**
         * Sets functional status
         */
        inline void setFunctional(bool functional)
        {
            _functional = functional;
        }

    private:

        /**
         * @brief the dbus object
         */
        sdbusplus::bus::bus& _bus;

        /**
         * @brief Reference to the parent Fan object
         */
        Fan& _fan;

        /**
         * @brief The name of the sensor, including the full path
         *
         * For example /xyz/openbmc_project/sensors/fan_tach/fan0
         */
        const std::string _name;

        /**
         * @brief If functional (not too slow).  The parent
         *        fan object sets this.
         */
        bool _functional = true;

        /**
         * @brief If the sensor has a Target property (can set speed)
         */
        const bool _hasTarget;

        /**
         * @brief The input speed, from the Value dbus property
         */
        int64_t _tachInput = 0;

        /**
         * @brief The current target speed, from the Target dbus property
         *        (if applicable)
         */
        uint64_t _tachTarget = 0;

        /**
         * @brief The timeout value to use on startup
         */
        size_t _startupTimeout;

        /**
         * @brief The timeout value to use during normal operation
         */
        const size_t _timeout;
};

}
}
}
