// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include <gpiod.hpp>

#include <string>

namespace phosphor::fan::monitor
{
/**
 * @class GPIO
 *
 * A base class to represent a GPIO (General Purpose Input/Output) pin.
 * Contains methods for reading/writing to the pin.
 */
class GPIO
{
  public:
    GPIO() = default;
    GPIO(const GPIO&) = delete;
    GPIO(GPIO&&) = delete;
    GPIO& operator=(const GPIO&) = delete;
    GPIO& operator=(GPIO&&) = delete;
    virtual ~GPIO() = default;

    /**
     * @brief Request to read from the GPIO.
     */
    virtual void requestRead() = 0;

    /**
     * @brief Request to write to the GPIO.
     *
     * @param[in] initialValue - The initial value to write to the GPIO
     */
    virtual void requestWrite(int initialValue) = 0;

    /**
     * @brief Get the value of the GPIO.
     *
     * @return - The value of the GPIO
     */
    virtual int getValue() = 0;

    /**
     * @brief Set the value of the GPIO.
     *
     * @param[in] value - The value to set the GPIO to
     */
    virtual void setValue(int value) = 0;

    /**
     * @brief Release ownership of the GPIO
     */
    virtual void release() = 0;
};

/**
 * @class BMCGPIO
 *
 * A class to represent a GPIO pin on a BMC. Implements methods using libgpiod.
 */
class BMCGPIO : public GPIO
{
  public:
    BMCGPIO(const BMCGPIO&) = delete;
    BMCGPIO(BMCGPIO&&) = delete;
    BMCGPIO& operator=(const BMCGPIO&) = delete;
    BMCGPIO& operator=(BMCGPIO&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] name - The name of the GPIO
     */
    explicit BMCGPIO(const std::string& name)
    {
        line = gpiod::find_line(name);
        if (!line)
        {
            throw std::invalid_argument{
                "Invalid Fan Presence GPIO name: " + name};
        }
    }

    /**
     * @brief Destructor
     *
     * Needed to release ownership of the GPIO upon destruction of the object.
     */
    ~BMCGPIO()
    {
        try
        {
            if (line.is_requested())
            {
                release();
            }
        }
        catch (...)
        {}
    }

    /** @copydoc GPIO::requestRead() */
    void requestRead() override
    {
        line.request({consumer, gpiod::line_request::DIRECTION_INPUT, 0});
    }

    /** @copydoc GPIO::requestWrite() */
    void requestWrite(int initialValue) override
    {
        line.request({consumer, gpiod::line_request::DIRECTION_OUTPUT, 0},
                     initialValue);
    }

    /** @copydoc GPIO::getValue() */
    int getValue() override
    {
        return line.get_value();
    }

    /** @copydoc GPIO::setValue() */
    void setValue(int value) override
    {
        line.set_value(value);
    }

    /** @copydoc GPIO::release() */
    void release() override
    {
        line.release();
    }

  private:
    /**
     * @brief Consumer value provided when requesting a GPIO line
     */
    inline static const std::string consumer{"phosphor-fan-monitor"};

    /**
     * @brief GPIO line used to access the pin
     */
    gpiod::line line;
};
} // namespace phosphor::fan::monitor
