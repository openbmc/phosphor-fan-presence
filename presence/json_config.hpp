#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

#include "config.h"
#include "rpolicy.hpp"
#include "fan.hpp"

namespace phosphor
{
namespace fan
{
namespace presence
{

using json = nlohmann::json;
using policies = std::vector<std::unique_ptr<RedundancyPolicy>>;

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
        std::vector<Fan> _fans;

        /**
         * @brief Process the json config to extract the defined fan presence
         * policies.
         */
        void process();
};

} // namespace presence
} // namespace fan
} // namespace phosphor
