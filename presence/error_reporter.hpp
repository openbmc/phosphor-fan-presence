#pragma once

#include "fan.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>

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
     * @brief Reference to the D-Bus connection object.
     */
    sdbusplus::bus::bus& _bus;

    /**
     * @brief The amount of time in seconds that a fan must be missing
     *        before an event log is created for it.
     */
    std::chrono::seconds _fanMissingErrorTime;
};

} // namespace phosphor::fan::presence
