#pragma once

#include "tach_sensor.hpp"

#include <chrono>
#include <unordered_set>

namespace phosphor::fan::monitor
{

class System;

/**
 * @class MalfunctionMonitor
 *
 * This class attempts to fix a max31785 fan controller malfunction
 * where it mysteriously emits unrealistically high tach readings.
 *
 * Typically in the field the previous captured readings have
 * alternated between exactly 29104 and zero.
 *
 * It will trigger on a high tach reading, specified in a JSON
 * config, and will then reset the fan controller with the specified
 * GPIO to try to recover.
 *
 * It won't allow another reset until resetState() is called.
 */
class MalfunctionMonitor
{
  public:
    MalfunctionMonitor() = delete;
    ~MalfunctionMonitor() = default;
    MalfunctionMonitor(const MalfunctionMonitor&) = delete;
    MalfunctionMonitor& operator=(const MalfunctionMonitor&) = delete;
    MalfunctionMonitor(MalfunctionMonitor&&) = delete;
    MalfunctionMonitor& operator=(MalfunctionMonitor&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] system - The System object
     * @param[in] limit - The max tach limit to identify the malfunction
     * @param[in] resetGPIO - gpiod GPIO name to do the reset
     */
    MalfunctionMonitor(System& system, double limit,
                       std::string_view resetGPIO);

    /**
     * @brief Checks if the sensor's tach reading is showing evidence of a
     *        malfunction and attempts to recover from it by resetting the
     *        fan controller, if a reset hasn't already been done.
     *
     * @param[in] sensor - The sensor to check
     *
     * @return bool - true if malfunction detected and reset done, false else
     */
    bool checkAndAttemptRecovery(const TachSensor& sensor);

    /**
     * @brief Checks if a sensor on the fan specified has shown evidence
     *        of a malfunction.
     *
     * @param[in] fanName - the name of the fan to check
     *
     * @return bool - true if the fan is affected, false else
     */
    bool isFanAffected(const std::string& fanName) const
    {
        return affectedFans.contains(fanName);
    }

    /**
     * @brief Resets the object back to a no malfunction state.
     */
    inline void resetState()
    {
        resetDone = false;
        affectedFans.clear();
    }

  private:
    /**
     * @brief Resets the fan control device by toggling a GPIO
     */
    void resetFanController();

    /**
     * @brief Says if the malfunction is detected on the sensor
     *
     * @param[in] sensor - The sensor to check
     *
     * @return bool - true if detected, false else
     */
    inline bool malfunctionDetected(const TachSensor& sensor) const
    {
        return sensor.getInput() >= tachLimit;
    }

    /**
     * @brief Creates an informational event log stating that the
     *       reset occurred.
     *
     * @param[in] sensor - The sensor first detecting the malfunction
     *                     leading to the reset.
     */
    void logResetError(const TachSensor& sensor) const;

    /**
     * @brief Reference to the System object
     */
    System& system;

    /**
     * @brief The tach limit used to identify the malfunction
     */
    double tachLimit;

    /**
     * @brief The GPIO to use to do the reset
     */
    std::string resetGPIO;

    /**
     * @brief A list of all fans affected by the malfunction in the current
     *        reset period.
     */
    std::unordered_set<std::string> affectedFans;

    /**
     * @brief If a reset was already done this power on.
     */
    bool resetDone = false;
};

} // namespace phosphor::fan::monitor
