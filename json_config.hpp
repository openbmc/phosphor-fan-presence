/**
 * Copyright © 2020 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "sdbusplus.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/source/signal.hpp>

#include <filesystem>
#include <format>
#include <fstream>

namespace phosphor::fan
{

namespace fs = std::filesystem;
using json = nlohmann::json;
using namespace phosphor::logging;

constexpr auto confOverridePath = "/etc/phosphor-fan-presence";
constexpr auto confBasePath = "/usr/share/phosphor-fan-presence";
constexpr auto entityManagerServ = "xyz.openbmc_project.EntityManager";
#ifdef USE_IBM_COMPATIBLE_SYSTEM
constexpr auto confCompatIntf =
    "xyz.openbmc_project.Configuration.IBMCompatibleSystem";
constexpr auto confCompatProp = "Names";
#endif
// PDI description: Implement to provide basic item attributes.
// Required by all objects within the inventory namespace.
constexpr auto invItemIntf = "xyz.openbmc_project.Inventory.Item";
constexpr auto prettyNameProp = "PrettyName";

/**
 * @class NoConfigFound - A no JSON configuration found exception
 *
 * A no JSON configuration found exception that is used to denote that a JSON
 * configuration has not been found yet.
 */
class NoConfigFound : public std::runtime_error
{
  public:
    NoConfigFound() = delete;
    NoConfigFound(const NoConfigFound&) = delete;
    NoConfigFound(NoConfigFound&&) = delete;
    NoConfigFound& operator=(const NoConfigFound&) = delete;
    NoConfigFound& operator=(NoConfigFound&&) = delete;
    ~NoConfigFound() = default;

    /**
     * @brief No JSON configuration found exception object
     *
     * When attempting to find the JSON configuration file(s), a NoConfigFound
     * exception can be thrown to denote that at that time finding/loading the
     * JSON configuration file(s) for a fan application failed. Details on what
     * application and JSON configuration file that failed to be found will be
     * logged resulting in the application being terminated.
     *
     * @param[in] details - Additional details
     */
    NoConfigFound(const std::string& appName, const std::string& fileName) :
        std::runtime_error(std::format("JSON configuration not found [Could "
                                       "not find fan {} conf file {}]",
                                       appName, fileName)
                               .c_str())
    {}
};

class JsonConfig
{
  public:
    /**
     * @brief Get the object paths with the input target interface
     *
     * Retrieve all the object paths implementing the target interface for
     * configuration file loading.
     */
    auto getTargetObjPaths(const std::string& targetInf)
    {
        auto paths = util::SDBusPlus::getSubTreePathsRaw(
            util::SDBusPlus::getBus(), "/", targetInf, 0);
        return paths;
    }

    /**
     * @brief Get property values with the target interface and property
     *
     * Retrieve all the target property values of the object paths implementing
     * the target interface
     */
    template <typename T>
    std::vector<T> getTargetProperties(const std::string& targetInf,
                                       const std::string& targetProp)
    {
        std::vector<std::string> objPaths;
        std::vector<T> output;
        try
        {
            objPaths = getTargetObjPaths(targetInf);
        }
        catch (const util::DBusMethodError&)
        {
            // Target interface does not exist on any dbus object yet
        }

        if (!objPaths.empty())
        {
            for (auto& path : objPaths)
            {
                try
                {
                    output.emplace_back(util::SDBusPlus::getProperty<T>(
                        util::SDBusPlus::getBus(), path, targetInf,
                        targetProp));
                }
                catch (const util::DBusError&)
                {
                    // Target property unavailable on this dbus object
                    // path's target interface, ignore
                }
            }
        }
        return output;
    }

    /**
     * @brief Constructor
     *
     * Attempts to set the list of values from the IBM compatible interface
     * (if configured) and the inventory item interface
     * and call the fan app's function to load its config file(s). If
     * both interface are not found, it subscribes to the
     * interfacesAdded signal for that interface on the EntityManager service
     * defined above.
     *
     * @param[in] func - Fan app function to call to load its config file(s)
     */
    JsonConfig(std::function<void()> func) : _loadFunc(func)
    {
        _match = std::make_unique<sdbusplus::bus::match_t>(
            util::SDBusPlus::getBus(),
            sdbusplus::bus::match::rules::interfacesAdded() +
                sdbusplus::bus::match::rules::sender(entityManagerServ),
            std::bind(&JsonConfig::targetIntfAdded, this,
                      std::placeholders::_1));

#ifdef USE_IBM_COMPATIBLE_SYSTEM
        auto propVals = getTargetProperties<std::vector<std::string>>(
            confCompatIntf, confCompatProp);
        if (!propVals.empty())
        {
            // last one found will be what's used if more
            // than one dbus object implementing the compatible
            // interface exists
            _confCompatVals = propVals.back();
        }
#endif
        // archive all the results to look for the correct one later
        _prettyNameVals = getTargetProperties<std::string>(invItemIntf,
                                                           prettyNameProp);

        // Check if required config(s) are found, otherwise this is intended to
        // catch the exception thrown by the getConfFile function when the
        // required config file was not found. This would then result in waiting
        // for the compatible interfacesAdded signal
        try
        {
            _loadFunc();
        }
        catch (const NoConfigFound&)
        {
            // Wait for compatible interfacesAdded signal
        }
    }

    /**
     * @brief InterfacesAdded callback function for the needed interface.
     *
     * @param[in] msg - The D-Bus message contents
     *
     * If the target interface is found, it uses the target property on
     * the interface to set the list of property values to be used when
     * attempting to get a configuration file. Once the list of value
     * has been updated, it calls the load function. If IBM compatible
     * interface of IBM is configured to be used but not found, rely on
     * the inventory item interface. If the configs have been loaded,
     * it won't process further.
     */
    void targetIntfAdded(sdbusplus::message_t& msg)
    {
        // Avoid processing any interface added when there's already a
        // valid sub-directory name to look for configs. _systemName is
        // cleared when getConfFile() fails to find configs under this
        // folder name.
        if (!_systemName.empty())
        {
            return;
        }

        sdbusplus::message::object_path op;
        std::map<std::string,
                 std::map<std::string,
                          std::variant<std::vector<std::string>, std::string>>>
            intfProps;

        msg.read(op, intfProps);

#ifdef USE_IBM_COMPATIBLE_SYSTEM
        if (intfProps.find(confCompatIntf) != intfProps.end())
        {
            const auto& props = intfProps.at(confCompatIntf);
            // Only one dbus object with the compatible interface is used at a
            // time
            _confCompatVals =
                std::get<std::vector<std::string>>(props.at(confCompatProp));
            try
            {
                _loadFunc();
                return;
            }
            catch (const NoConfigFound&)
            {
                // No config found - look for the inventory item interface
            }
        }
#endif
        if (intfProps.find(invItemIntf) == intfProps.end())
        {
            // Can't find inventory item interface;
            return;
        }
        const auto& props = intfProps.at(invItemIntf);
        std::string propVal = std::get<std::string>(props.at(prettyNameProp));
        _prettyNameVals.emplace_back(propVal);

        // Let it throw here if no config is found after many efforts
        _loadFunc();
    }

    /**
     * Get the json configuration file. The first location found to contain
     * the json config file for the given fan application is used from the
     * following locations in order.
     * 1.) From the confOverridePath location
     * 2.) From the default confBasePath location
     * 3.) From config file found using an entry from a list obtained from an
     * interface's property as a relative path extension on the base path where:
     * if USE_IBM_COMPATIBLE_SYSTEM is defined:
     *     interface = Interface set in confCompatIntf with the property
     *     property = Property set in confCompatProp containing a list of
     *                subdirectories in priority order to find a config
     * else:
     *     interface = Interface set in invItemIntf with the property
     *     property = Property set in prettyNameProp containing a string
     *                of subdirectory
     * @brief Get the configuration file to be used
     *
     * @param[in] appName - The phosphor-fan-presence application name
     * @param[in] fileName - Application's configuration file's name
     * @param[in] isOptional - Config file is optional, default to 'false'
     *
     * @return filesystem path
     *     The filesystem path to the configuration file to use
     */
    static const fs::path getConfFile(const std::string& appName,
                                      const std::string& fileName,
                                      bool isOptional = false)
    {
        // Check override location
        fs::path confFile = fs::path{confOverridePath} / appName / fileName;
        if (fs::exists(confFile))
        {
            return confFile;
        }

        // If the default file is there, use it
        confFile = fs::path{confBasePath} / appName / fileName;
        if (fs::exists(confFile))
        {
            return confFile;
        }

#ifdef USE_IBM_COMPATIBLE_SYSTEM
        // Look for a config file at each entry relative to the base
        // path and use the first one found
        auto confCompatValsIt =
            std::find_if(_confCompatVals.begin(), _confCompatVals.end(),
                         [&confFile, &appName, &fileName](const auto& value) {
            confFile = fs::path{confBasePath} / appName / value / fileName;
            _systemName = value;
            return fs::exists(confFile);
            });
        if (confCompatValsIt != _confCompatVals.end())
        {
            return confFile;
        }
        // Couldn't find configs from IBM compatible interface,
        // rely on the inventory item interface
        confFile.clear();
        _systemName.clear();
#endif
        // Look for config path among the list of achieved inventory item pretty
        // names
        auto prettyNameValsIt =
            std::find_if(_prettyNameVals.begin(), _prettyNameVals.end(),
                         [&confFile, &appName, &fileName](const auto& value) {
            confFile = fs::path{confBasePath} / appName / value / fileName;
            _systemName = value;
            return fs::exists(confFile);
            });

        if (prettyNameValsIt == _prettyNameVals.end())
        {
            confFile.clear();
            _systemName.clear();
        }

        if (confFile.empty() && !isOptional)
        {
            throw NoConfigFound(appName, fileName);
        }

        return confFile;
    }

    /**
     * @brief Load the JSON config file
     *
     * @param[in] confFile - File system path of the configuration file to load
     *
     * @return Parsed JSON object
     *     The parsed JSON configuration file object
     */
    static const json load(const fs::path& confFile)
    {
        std::ifstream file;
        json jsonConf;

        if (!confFile.empty() && fs::exists(confFile))
        {
            log<level::INFO>(
                std::format("Loading configuration from {}", confFile.string())
                    .c_str());
            file.open(confFile);
            try
            {
                // Enable ignoring `//` or `/* */` comments
                jsonConf = json::parse(file, nullptr, true, true);
            }
            catch (const std::exception& e)
            {
                log<level::ERR>(
                    std::format(
                        "Failed to parse JSON config file: {}, error: {}",
                        confFile.string(), e.what())
                        .c_str());
                throw std::runtime_error(
                    std::format(
                        "Failed to parse JSON config file: {}, error: {}",
                        confFile.string(), e.what())
                        .c_str());
            }
        }
        else
        {
            log<level::ERR>(std::format("Unable to open JSON config file: {}",
                                        confFile.string())
                                .c_str());
            throw std::runtime_error(
                std::format("Unable to open JSON config file: {}",
                            confFile.string())
                    .c_str());
        }

        return jsonConf;
    }

    /**
     * @brief Return the compatible values property
     *
     * @return const std::vector<std::string>& - The values
     */
    static const std::vector<std::string>& getCompatValues()
    {
        return _confCompatVals;
    }

  private:
    /* Load function to call for a fan app to load its config file(s). */
    std::function<void()> _loadFunc;

    /**
     * @brief The interfacesAdded match that is used to wait
     *        for the IBMCompatibleSystem interface to show up.
     */
    std::unique_ptr<sdbusplus::bus::match_t> _match;

    /**
     * @brief List of compatible values from the compatible interface
     *
     * Only supports a single instance of the compatible interface on a dbus
     * object. If more than one dbus object exists with the compatible
     * interface, the last one found will be the list of compatible values used.
     */
    inline static std::vector<std::string> _confCompatVals;

    /**
     * @brief List of pretty name values from the inventory item interface
     *
     * If more than one dbus object exists with the inventory item interface,
     * all will be archived to look for the correct one later (the one that
     * contains the config files when acting as a sub-directory).
     */

    inline static std::vector<std::string> _prettyNameVals;

    /**
     * @brief The property value that is currently used as the system location
     *
     * The value extracted from the achieved property value list that is used
     * as a system location to append on each config file as a sub-directory to
     * look for the config files and really contains them.
     */

    inline static std::string _systemName;
};

} // namespace phosphor::fan
