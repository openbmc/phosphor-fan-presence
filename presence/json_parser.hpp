#pragma once

#include "error_reporter.hpp"
#include "fan.hpp"
#include "psensor.hpp"
#include "rpolicy.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/source/signal.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace phosphor
{
namespace fan
{
namespace presence
{

namespace fs = std::filesystem;
using json = nlohmann::json;

constexpr auto confFileName = "config.json";
constexpr auto confAppName = "presence";

using policies = std::vector<std::unique_ptr<RedundancyPolicy>>;

constexpr auto fanPolicyFanPos = 0;
constexpr auto fanPolicySensorListPos = 1;
using fanPolicy = std::tuple<Fan, std::vector<std::unique_ptr<PresenceSensor>>>;

// Presence method handler function
using methodHandler =
    std::function<std::unique_ptr<PresenceSensor>(size_t, const json&)>;
// Presence redundancy policy handler function
using rpolicyHandler = std::function<std::unique_ptr<RedundancyPolicy>(
    const fanPolicy&, std::unique_ptr<EEPROMDevice>)>;

class JsonConfig
{
  public:
    JsonConfig() = delete;
    JsonConfig(const JsonConfig&) = delete;
    JsonConfig(JsonConfig&&) = delete;
    JsonConfig& operator=(const JsonConfig&) = delete;
    JsonConfig& operator=(JsonConfig&&) = delete;
    ~JsonConfig() = default;

    /**
     * Constructor
     *
     * @param[in] bus - sdbusplus bus object
     */
    explicit JsonConfig(sdbusplus::bus_t& bus);

    /**
     * @brief Get the json config based fan presence policies
     *
     * @return - The fan presence policies
     */
    static const policies& get();

    /**
     * @brief Callback function to handle receiving a HUP signal to
     * reload the json configuration.
     *
     * @param[in] sigSrc - sd_event_source signal wrapper
     * @param[in] sigInfo - signal info on signal fd
     */
    void sighupHandler(sdeventplus::source::Signal& /*sigSrc*/,
                       const struct signalfd_siginfo* /*sigInfo*/);

    /**
     * @brief Parses and populates the fan presence policies from
     *        the json file and then starts the actual presence
     *        detecting.
     */
    void start();

  private:
    /* Fan presence policies */
    static policies _policies;

    /* The sdbusplus bus object */
    sdbusplus::bus_t& _bus;

    /* List of Fan objects to have presence policies */
    std::vector<fanPolicy> _fans;

    /* Presence methods mapping to their associated handler function */
    static const std::map<std::string, methodHandler> _methods;

    /**
     * Presence redundancy policy mapping to their associated handler
     * function
     */
    static const std::map<std::string, rpolicyHandler> _rpolicies;

    /**
     * Class that handles reporting errors for missing fans.
     */
    std::unique_ptr<ErrorReporter> _reporter;

    /**
     * Tracks if the config has already been loaded.
     */
    bool _loaded = false;

    /**
     * @brief Process the json config to extract the defined fan presence
     * policies.
     *
     * @param[in] jsonConf - parsed json configuration data
     */
    void process(const json& jsonConf);

    /**
     * @brief Get the redundancy policy of presence detection for a fan
     *
     * @param[in] rpolicy - policy type to construct
     * @param[in] fpolicy - fan policy object
     * @param[in] eepromDevice - EEPROM device object
     *
     * @return - The constructed redundancy policy type for the fan
     */
    std::unique_ptr<RedundancyPolicy>
        getPolicy(const json& rpolicy, const fanPolicy& fpolicy,
                  std::unique_ptr<EEPROMDevice> eepromDevice);
};

/**
 * Methods of fan presence detection function declarations
 */
namespace method
{
/**
 * @brief Fan presence detection method by tach feedback
 *
 * @param[in] fanIndex - fan object index to add tach method
 * @param[in] method - json properties for a tach method
 *
 * @return - A presence sensor to detect fan presence by tach feedback
 */
std::unique_ptr<PresenceSensor> getTach(size_t fanIndex, const json& method);

/**
 * @brief Fan presence detection method by gpio
 *
 * @param[in] fanIndex - fan object index to add gpio method
 * @param[in] method - json properties for a gpio method
 *
 * @return - A presence sensor to detect fan presence by gpio
 */
std::unique_ptr<PresenceSensor> getGpio(size_t fanIndex, const json& method);

} // namespace method

/**
 * Redundancy policies for fan presence detection function declarations
 */
namespace rpolicy
{
/**
 * @brief Create an `Anyof` redundancy policy on the created presence
 * sensors for a fan
 *
 * @param[in] fan - fan policy object with the presence sensors for the fan
 * @param[in] eepromDevice - EEPROM device object
 *
 * @return - An `Anyof` redundancy policy
 */
std::unique_ptr<RedundancyPolicy>
    getAnyof(const fanPolicy& fan, std::unique_ptr<EEPROMDevice> eepromDevice);

/**
 * @brief Create a `Fallback` redundancy policy on the created presence
 * sensors for a fan
 *
 * @param[in] fan - fan policy object with the presence sensors for the fan
 * @param[in] eepromDevice - EEPROM device object
 *
 * @return - A `Fallback` redundancy policy
 */
std::unique_ptr<RedundancyPolicy>
    getFallback(const fanPolicy& fan,
                std::unique_ptr<EEPROMDevice> eepromDevice);

} // namespace rpolicy

} // namespace presence
} // namespace fan
} // namespace phosphor
