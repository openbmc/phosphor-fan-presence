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

#include <fmt/format.h>

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/source/signal.hpp>

#include <filesystem>
#include <fstream>

namespace phosphor::fan
{

namespace fs = std::filesystem;
using json = nlohmann::json;
using namespace phosphor::logging;

constexpr auto confOverridePath = "/etc/phosphor-fan-presence";
constexpr auto confBasePath = "/usr/share/phosphor-fan-presence";
constexpr auto confCompatServ = "xyz.openbmc_project.EntityManager";
constexpr auto confCompatIntf =
    "xyz.openbmc_project.Configuration.IBMCompatibleSystem";
constexpr auto confCompatProp = "Names";

class JsonConfig
{
  public:
    using ConfFileReadyFunc = std::function<void(const std::string&)>;

    /**
     * @brief Get the object paths with the compatible interface
     *
     * Retrieve all the object paths implementing the compatible interface for
     * configuration file loading.
     */
    static auto& getCompatObjPaths() __attribute__((pure))
    {
        static auto paths = util::SDBusPlus::getSubTreePathsRaw(
            util::SDBusPlus::getBus(), "/", confCompatIntf, 0);
        return paths;
    }

    /**
     * @brief Constructor
     *
     * Looks for the JSON config file.  If it can't find one, then it
     * will watch entity-manager for the IBMCompatibleSystem interface
     * to show up and then use that data to try again.  If the config
     * file is initially present, the callback function is executed
     * with the path to the file.
     *
     * @param[in] bus - The dbus bus object
     * @param[in] appName - The appName portion of the config file path
     * @param[in] fileName - Application's configuration file's name
     * @param[in] func - The function to call when the config file
     *                   is found.
     */
    JsonConfig(sdbusplus::bus::bus& bus, const std::string& appName,
               const std::string& fileName, ConfFileReadyFunc func) :
        _appName(appName),
        _fileName(fileName), _readyFunc(func)
    {
        _match = std::make_unique<sdbusplus::server::match::match>(
            bus,
            sdbusplus::bus::match::rules::interfacesAdded() +
                sdbusplus::bus::match::rules::sender(
                    "xyz.openbmc_project.EntityManager"),
            std::bind(&JsonConfig::ifacesAddedCallback, this,
                      std::placeholders::_1));
        try
        {
            auto compatObjPaths = getCompatObjPaths();
            if (!compatObjPaths.empty())
            {
                for (auto& path : compatObjPaths)
                {
                    try
                    {
                        // Retrieve json config compatible relative path
                        // locations (last one found will be what's used if more
                        // than one dbus object implementing the comptaible
                        // interface exists).
                        _confCompatValues = util::SDBusPlus::getProperty<
                            std::vector<std::string>>(bus, path, confCompatIntf,
                                                      confCompatProp);
                    }
                    catch (const util::DBusError&)
                    {
                        // Property unavailable on this dbus object path.
                    }
                }
            }
            _confFile = getConfFile(bus, _appName, _fileName);
        }
        catch (const std::runtime_error& e)
        {
            // No conf file found, so let the interfacesAdded
            // match callback handle finding it.
        }

        if (!_confFile.empty())
        {
            _match.reset();
            _readyFunc(_confFile);
        }
    }

    /**
     * @brief Constructor
     *
     * Attempts to set the list of compatible values from the compatible
     * interface and call the fan app's function to load its config file(s). If
     * the compatible interface is not found, it subscribes to the
     * interfacesAdded signal for that interface on the compatible service
     * defined above.
     *
     * @param[in] func - Fan app function to call to load its config file(s)
     */
    JsonConfig(std::function<void()> func) : _loadFunc(func)
    {
        _match = std::make_unique<sdbusplus::server::match::match>(
            util::SDBusPlus::getBus(),
            sdbusplus::bus::match::rules::interfacesAdded() +
                sdbusplus::bus::match::rules::sender(confCompatServ),
            std::bind(&JsonConfig::compatIntfAdded, this,
                      std::placeholders::_1));
        try
        {
            auto compatObjPaths = getCompatObjPaths();
            if (!compatObjPaths.empty())
            {
                for (auto& path : compatObjPaths)
                {
                    try
                    {
                        // Retrieve json config compatible relative path
                        // locations (last one found will be what's used if more
                        // than one dbus object implementing the comptaible
                        // interface exists).
                        _confCompatValues = util::SDBusPlus::getProperty<
                            std::vector<std::string>>(util::SDBusPlus::getBus(),
                                                      path, confCompatIntf,
                                                      confCompatProp);
                    }
                    catch (const util::DBusError&)
                    {
                        // Compatible property unavailable on this dbus object
                        // path's compatible interface, ignore
                    }
                }
                _loadFunc();
                _match.reset();
            }
            else
            {
                // Check if required config(s) are found not needing the
                // compatible interface, otherwise this is intended to catch the
                // exception thrown by the getConfFile function when the
                // required config file was not found. This would then result in
                // waiting for the compatible interfacesAdded signal
                try
                {
                    _loadFunc();
                    _match.reset();
                }
                catch (const std::runtime_error&)
                {
                    // Wait for compatible interfacesAdded signal
                }
            }
        }
        catch (const std::runtime_error&)
        {
            // Wait for compatible interfacesAdded signal
        }
    }

    /**
     * @brief The interfacesAdded callback function that looks for
     *        the IBMCompatibleSystem interface.  If it finds it,
     *        it uses the Names property in the interface to find
     *        the JSON config file to use.  If it finds one, it calls
     *        the _readyFunc function with the config file path.
     *
     * @param[in] msg - The D-Bus message contents
     */
    void ifacesAddedCallback(sdbusplus::message::message& msg)
    {
        sdbusplus::message::object_path path;
        std::map<std::string,
                 std::map<std::string, std::variant<std::vector<std::string>>>>
            interfaces;

        msg.read(path, interfaces);

        if (interfaces.find(confCompatIntf) == interfaces.end())
        {
            return;
        }

        const auto& properties = interfaces.at(confCompatIntf);
        auto names =
            std::get<std::vector<std::string>>(properties.at(confCompatProp));

        auto it =
            std::find_if(names.begin(), names.end(), [this](auto const& name) {
                auto confFile =
                    fs::path{confBasePath} / _appName / name / _fileName;
                if (fs::exists(confFile))
                {
                    _confFile = confFile;
                    return true;
                }
                return false;
            });

        if (it != names.end())
        {
            _readyFunc(_confFile);
            _match.reset();
        }
        else
        {
            log<level::ERR>(fmt::format("Could not find fan {} conf file {} "
                                        "even after {} iface became available",
                                        _appName, _fileName, confCompatIntf)
                                .c_str());
        }
    }

    /**
     * @brief InterfacesAdded callback function for the compatible interface.
     *
     * @param[in] msg - The D-Bus message contents
     *
     * If the compatible interface is found, it uses the compatible property on
     * the interface to set the list of compatible values to be used when
     * attempting to get a configuration file. Once the list of compatible
     * values has been updated, it calls the load function.
     */
    void compatIntfAdded(sdbusplus::message::message& msg)
    {
        sdbusplus::message::object_path op;
        std::map<std::string,
                 std::map<std::string, std::variant<std::vector<std::string>>>>
            intfProps;

        msg.read(op, intfProps);

        if (intfProps.find(confCompatIntf) == intfProps.end())
        {
            return;
        }

        const auto& props = intfProps.at(confCompatIntf);
        // Only one dbus object with the compatible interface is used at a time
        _confCompatValues =
            std::get<std::vector<std::string>>(props.at(confCompatProp));
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
     *     interface = Interface set in confCompatIntf with the property
     *     property = Property set in confCompatProp containing a list of
     *                subdirectories in priority order to find a config
     *
     * @brief Get the configuration file to be used
     *
     * @param[in] bus - The dbus bus object
     * @param[in] appName - The phosphor-fan-presence application name
     * @param[in] fileName - Application's configuration file's name
     * @param[in] isOptional - Config file is optional, default to 'false'
     *
     * @return filesystem path
     *     The filesystem path to the configuration file to use
     */
    static const fs::path getConfFile(sdbusplus::bus::bus& bus,
                                      const std::string& appName,
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

        // TODO - Will be removed in a following commit in place of using a
        // constructor to populate the compatible values
        auto compatObjPaths = getCompatObjPaths();
        if (!compatObjPaths.empty())
        {
            for (auto& path : compatObjPaths)
            {
                try
                {
                    // Retrieve json config compatible relative path
                    // locations (last one found will be what's used if more
                    // than one dbus object implementing the comptaible
                    // interface exists).
                    _confCompatValues =
                        util::SDBusPlus::getProperty<std::vector<std::string>>(
                            bus, path, confCompatIntf, confCompatProp);
                }
                catch (const util::DBusError&)
                {
                    // Property unavailable on this dbus object path.
                }
            }
        }

        // Look for a config file at each entry relative to the base
        // path and use the first one found
        auto it = std::find_if(
            _confCompatValues.begin(), _confCompatValues.end(),
            [&confFile, &appName, &fileName](const auto& value) {
                confFile = fs::path{confBasePath} / appName / value / fileName;
                return fs::exists(confFile);
            });
        if (it == _confCompatValues.end())
        {
            confFile.clear();
        }

        if (!isOptional && confFile.empty() && !_confCompatValues.empty())
        {
            log<level::ERR>(fmt::format("Could not find fan {} conf file {}",
                                        appName, fileName)
                                .c_str());
        }

        if (confFile.empty() && !isOptional)
        {
            throw std::runtime_error("No JSON config file found");
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
                fmt::format("Loading configuration from {}", confFile.string())
                    .c_str());
            file.open(confFile);
            try
            {
                jsonConf = json::parse(file);
            }
            catch (std::exception& e)
            {
                log<level::ERR>(
                    fmt::format(
                        "Failed to parse JSON config file: {}, error: {}",
                        confFile.string(), e.what())
                        .c_str());
                throw std::runtime_error(
                    fmt::format(
                        "Failed to parse JSON config file: {}, error: {}",
                        confFile.string(), e.what())
                        .c_str());
            }
        }
        else
        {
            log<level::ERR>(fmt::format("Unable to open JSON config file: {}",
                                        confFile.string())
                                .c_str());
            throw std::runtime_error(
                fmt::format("Unable to open JSON config file: {}",
                            confFile.string())
                    .c_str());
        }

        return jsonConf;
    }

  private:
    /**
     * @brief The 'appName' portion of the config file path.
     */
    const std::string _appName;

    /**
     * @brief The config file name.
     */
    const std::string _fileName;

    /**
     * @brief The function to call when the config file is available.
     */
    ConfFileReadyFunc _readyFunc;

    /* Load function to call for a fan app to load its config file(s). */
    std::function<void()> _loadFunc;

    /**
     * @brief The JSON config file
     */
    fs::path _confFile;

    /**
     * @brief The interfacesAdded match that is used to wait
     *        for the IBMCompatibleSystem interface to show up.
     */
    std::unique_ptr<sdbusplus::server::match::match> _match;

    /**
     * @brief List of compatible values from the compatible interface
     *
     * Only supports a single instance of the compatible interface on a dbus
     * object. If more than one dbus object exists with the compatible
     * interface, the last one found will be the list of compatible values used.
     */
    inline static std::vector<std::string> _confCompatValues;
};

} // namespace phosphor::fan
