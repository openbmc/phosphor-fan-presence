#pragma once

#include <memory>
#include "utility.hpp"

namespace phosphor
{
namespace gpio
{


/**
 * Represents a GPIO.
 *
 * Operations are setting low or high.
 *
 * Plumbing for doing a read is present, but not the
 * public API because it doesn't return reliable values.
 */
class GPIO
{
    public:

        /**
         * If the GPIO is an input or output
         */
        enum class Direction
        {
            input,
            output
        };

        /**
         * The possible values - low or high
         */
        enum class Value
        {
            low,
            high
        };
        
        GPIO() = delete;
        GPIO(const GPIO&) = delete;
        GPIO(GPIO&&) = default;
        GPIO& operator=(const GPIO&) = delete;
        GPIO& operator=(GPIO&&) = default;
        ~GPIO() = default;

        /**
         * Constructor
         *
         * @param[in] device - the GPIO device file
         * @param[in] gpio - the GPIO number
         * @param[in] direction - the GPIO direction
         */
        GPIO(const std::string& device,
             unsigned int gpio,
             Direction direction) :
            _device(device),
            _gpio(gpio),
            _direction(direction)
        {
        }

        /**
         * Sets the GPIO high
         */
        inline void setHigh()
        {
            setGPIO(Value::high);
        }

        /**
         * Sets the GPIO low
         */
        inline void setLow()
        {
            setGPIO(Value::low);
        }

    private:

        /**
         * Requests a GPIO line from the GPIO device
         *
         * @param[in] defaultValue - The default value, required for
         *                           output GPIOs only.
         */
        void requestLine(Value defaultValue = Value::high);

        /**
         * Sets the GPIO to low or high
         *
         * Requests the GPIO line if it hasn't been done already.
         */
        void setGPIO(Value value);

        /**
         * The GPIO device name, like /dev/gpiochip0
         */
        const std::string _device;

        /**
         * The GPIO number
         */
        const unsigned int _gpio;

        /**
         * The GPIO direction
         */
        const Direction _direction;

        /**
         * File descriptor for the GPIO line
         */
        std::unique_ptr<phosphor::fan::util::FileDescriptor> _lineFD;
};

}
}
