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
// Presence method handler function
using methodHandler = std::function<
    std::unique_ptr<PresenceSensor>(size_t, const json&)>;

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
        static const policies& get();

    private:

        /* Fan presence policies */
        static policies _policies;

        /* List of Fan objects to have presence policies */
        std::vector<Fan> _fans;

        /* Presence methods mapping to their associated handler function */
        static const std::map<std::string, methodHandler> _methods;

        /* List of fan presence sensors */
        std::vector<std::unique_ptr<PresenceSensor>> _sensors;

        /**
         * @brief Process the json config to extract the defined fan presence
         * policies.
         *
         * @param[in] jsonConf - parsed json configuration data
         */
        void process(const json& jsonConf);
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

} // namespace presence
} // namespace fan
} // namespace phosphor
