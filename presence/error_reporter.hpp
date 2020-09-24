#pragma once

#include "fan.hpp"
#include "power_state.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/utility/timer.hpp>

namespace phosphor::fan::presence
{
class PresenceSensor;

/**
 * @class ErrorReporter
 *
 * This class will create event logs for missing fans.  The logs are
 * created after a fan has been missing for an amount of time
 * specified in the JSON config file while power is on.
 *
 * The timers to create event logs are not started when power is off.
 * When power is turned on, the timers for any missing fans will be
 * be started.  If any timers are running when power is turned off,
 * they will be stopped.
 */
class ErrorReporter
{
  public:
    ErrorReporter() = delete;
    ~ErrorReporter() = default;
    ErrorReporter(const ErrorReporter&) = delete;
    ErrorReporter& operator=(const ErrorReporter&) = delete;
    ErrorReporter(ErrorReporter&&) = delete;
    ErrorReporter& operator=(ErrorReporter&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] bus - The sdbusplus bus object
     * @param[in] jsonConf - The 'reporting' section of the JSON config file
     * @param[in] fans - The fans for this configuration
     */
    ErrorReporter(
        sdbusplus::bus::bus& bus, const nlohmann::json& jsonConf,
        const std::vector<
            std::tuple<Fan, std::vector<std::unique_ptr<PresenceSensor>>>>&
            fans);

  private:
    /**
     * @brief Reads in the configuration from the JSON section
     *
     * @param[in] jsonConf - The 'reporting' section of the JSON
     */
    void loadConfig(const nlohmann::json& jsonConf);

    /**
     * @brief The propertiesChanged callback for the interface that
     *        contains the Present property of a fan.
     *
     * Will start the timer to create an event log if power is on.
     *
     * @param[in] msg - The payload of the propertiesChanged signal
     */
    void presenceChanged(sdbusplus::message::message& msg);

    /**
     * @brief The callback function called by the PowerState class
     *        when the power state changes.
     *
     * Will start timers for missing fans if power is on, and stop
     * them when power changes to off.
     *
     * @param[in] powerState - The new power state
     */
    void powerStateChanged(bool powerState);

    /**
     * @brief Called when the fan missing timer expires to create
     *        an event log for a missing fan.
     *
     * @param[in] fanPath - The D-Bus path of the missing fan.
     */
    void fanMissingTimerExpired(const std::string& fanPath);

    /**
     * @brief Checks if the fan missing timer for a fan needs to
     *        either be started or stopped based on the power and
     *        presence states.
     *
     * @param[in] fanPath - The D-Bus path of the fan
     */
    void checkFan(const std::string& fanPath);

    /**
     * @brief Reference to the D-Bus connection object.
     */
    sdbusplus::bus::bus& _bus;

    /**
     * @brief The connection to the event loop for the timer.
     */
    sdeventplus::Event _event;

    /**
     * @brief The propertiesChanged match objects.
     */
    std::vector<sdbusplus::bus::match::match> _matches;

    /**
     * @brief Base class pointer to the power state implementation.
     */
    std::unique_ptr<PowerState> _powerState;

    /**
     * @brief The amount of time in seconds that a fan must be missing
     *        before an event log is created for it.
     */
    std::chrono::seconds _fanMissingErrorTime;

    /**
     * @brief The map of fan paths to their presence states.
     */
    std::map<std::string, bool> _fanStates;

    /**
     * @brief The map of fan paths to their Timer objects.
     */
    std::map<std::string, std::unique_ptr<sdeventplus::utility::Timer<
                              sdeventplus::ClockId::Monotonic>>>
        _fanMissingTimers;
};

} // namespace phosphor::fan::presence
