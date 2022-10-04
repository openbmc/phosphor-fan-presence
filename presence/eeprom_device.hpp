#pragma once

#include "sdeventplus.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <filesystem>
#include <fstream>
#include <string>

namespace phosphor::fan::presence
{
using namespace phosphor::fan::util;

/**
 * @class EEPROMDevice
 *
 * Provides an API to bind an EEPROM driver to a device, after waiting
 * a configurable amount of time in case the device needs time
 * to initialize after being plugged into a system.
 */
class EEPROMDevice
{
  public:
    EEPROMDevice() = delete;
    ~EEPROMDevice() = default;
    EEPROMDevice(const EEPROMDevice&) = delete;
    EEPROMDevice& operator=(const EEPROMDevice&) = delete;
    EEPROMDevice(EEPROMDevice&&) = delete;
    EEPROMDevice& operator=(EEPROMDevice&&) = delete;

    /**
     * @brief Constructor
     * @param[in] address - The bus-address string as used by drivers
     *                      in sysfs.
     * @param[in] driver - The I2C driver name in sysfs
     * @param[in] bindDelayInMS - The time in milliseconds to wait
     *            before actually doing the bind.
     */
    EEPROMDevice(const std::string& address, const std::string& driver,
                 size_t bindDelayInMS) :
        address(address),
        path(baseDriverPath / driver), bindDelay(bindDelayInMS),
        timer(SDEventPlus::getEvent(),
              std::bind(std::mem_fn(&EEPROMDevice::bindTimerExpired), this))
    {}

    /**
     * @brief Kicks off the timer to do the actual bind
     */
    void bind()
    {
        timer.restartOnce(std::chrono::milliseconds{bindDelay});
    }

    /**
     * @brief Stops the bind timer if running and unbinds the device
     */
    void unbind()
    {
        if (timer.isEnabled())
        {
            timer.setEnabled(false);
        }

        unbindDevice();
    }

  private:
    /**
     * @brief When the bind timer expires it will bind the device.
     */
    void bindTimerExpired() const
    {
        unbindDevice();

        auto bindPath = path / "bind";
        std::ofstream bind{bindPath};
        if (bind.good())
        {
            lg2::info("Binding fan EEPROM device with address {ADDRESS}",
                      "ADDRESS", address);
            bind << address;
        }

        if (bind.fail())
        {
            lg2::error("Error while binding fan EEPROM device with path {PATH}"
                       " and address {ADDR}",
                       "PATH", bindPath, "ADDR", address);
        }
    }

    /**
     * @brief Unbinds the device.
     */
    void unbindDevice() const
    {
        auto devicePath = path / address;
        if (!std::filesystem::exists(devicePath))
        {
            return;
        }

        auto unbindPath = path / "unbind";
        std::ofstream unbind{unbindPath};
        if (unbind.good())
        {
            unbind << address;
        }

        if (unbind.fail())
        {
            lg2::error("Error while unbinding fan EEPROM device with path"
                       " {PATH} and address {ADDR}",
                       "PATH", unbindPath, "ADDR", address);
        }
    }

    /** @brief The base I2C drivers directory in sysfs */
    const std::filesystem::path baseDriverPath{"/sys/bus/i2c/drivers"};

    /**
     * @brief The address string with the i2c bus and address.
     * Example: '32-0050'
     */
    const std::string address;

    /** @brief The path to the driver dir, like /sys/bus/i2c/drivers/at24 */
    const std::filesystem::path path;

    /** @brief Number of milliseconds to delay to actually do the bind. */
    const size_t bindDelay{};

    /** @brief The timer to do the delay with */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> timer;
};

} // namespace phosphor::fan::presence
