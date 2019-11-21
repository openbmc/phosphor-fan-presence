#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

#include "config.h"
#include "rpolicy.hpp"
#include "fan.hpp"
#include "psensor.hpp"

namespace phosphor
{
namespace fan
{
namespace presence
{

using json = nlohmann::json;
using policies = std::vector<std::unique_ptr<RedundancyPolicy>>;

constexpr auto fanPolicyFanPos = 0;
constexpr auto fanPolicySensorListPos = 1;
using fanPolicy = std::tuple<Fan, std::vector<std::unique_ptr<PresenceSensor>>>;

// Presence method handler function
using methodHandler = std::function<
    std::unique_ptr<PresenceSensor>(size_t, const json&)>;
// Presence redundancy policy handler function
using rpolicyHandler = std::function<
    std::unique_ptr<RedundancyPolicy>(const fanPolicy&)>;

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
         * Parses and populates the fan presence policies from a json file
         *
         * @param[in] jsonFile - json configuration file
         */
        explicit JsonConfig(const std::string& jsonFile);

        /**
         * @brief Get the json config based fan presence policies
         *
         * @return - The fan presence policies
         */
        static const policies& getPolicies();

    private:

        /* Fan presence policies */
        static policies _policies;

        /* Parsed json configuration */
        json _jsonConf;

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
         * @brief Process the json config to extract the defined fan presence
         * policies.
         */
        void process();

        /**
         * @brief Add to the list of policies of presence detection
         *
         * @param[in] rpolicy - policy to add
         */
        void addPolicy(const json& rpolicy);
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
    std::unique_ptr<PresenceSensor> getTach(size_t fanIndex,
                                            const json& method);

    /**
     * @brief Fan presence detection method by gpio
     *
     * @param[in] fanIndex - fan object index to add gpio method
     * @param[in] method - json properties for a gpio method
     *
     * @return - A presence sensor to detect fan presence by gpio
     */
    std::unique_ptr<PresenceSensor> getGpio(size_t fanIndex,
                                            const json& method);

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
     *
     * @return - An `Anyof` redundancy policy
     */
    std::unique_ptr<RedundancyPolicy> getAnyof(const fanPolicy& fan);

    /**
     * @brief Create a `Fallback` redundancy policy on the created presence
     * sensors for a fan
     *
     * @param[in] fan - fan policy object with the presence sensors for the fan
     *
     * @return - A `Fallback` redundancy policy
     */
    std::unique_ptr<RedundancyPolicy> getFallback(const fanPolicy& fan);

} // namespace policy

} // namespace presence
} // namespace fan
} // namespace phosphor
