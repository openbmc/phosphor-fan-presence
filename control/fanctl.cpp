/**
 * Copyright © 2022 IBM Corporation
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

#include "config.h"

#include "sdbusplus.hpp"

#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>

#include <filesystem>
#include <iomanip>
#include <iostream>

using SDBusPlus = phosphor::fan::util::SDBusPlus;

constexpr auto systemdMgrIface = "org.freedesktop.systemd1.Manager";
constexpr auto systemdPath = "/org/freedesktop/systemd1";
constexpr auto systemdService = "org.freedesktop.systemd1";
constexpr auto phosphorServiceName = "phosphor-fan-control@0.service";
constexpr auto dumpFile = "/tmp/fan_control_dump.json";

enum
{
    FAN_NAMES = 0,
    PATH_MAP = 1,
    IFACES = 2,
    METHOD = 3
};

struct DumpQuery
{
    std::string section;
    std::string name;
    std::vector<std::string> properties;
    bool dump{false};
};

struct SensorOpts
{
    std::string type;
    std::string name;
    bool verbose{false};
};

struct SensorOutput
{
    std::string name;
    double value;
    bool functional;
    bool available;
};

/**
 * @function extracts fan name from dbus path string (last token where
 * delimiter is the / character), with proper bounds checking.
 * @param[in] path - D-Bus path
 * @return just the fan name.
 */

std::string justFanName(const std::string& path)
{
    std::string fanName;

    auto itr = path.rfind("/");
    if (itr != std::string::npos && itr < path.size())
    {
        fanName = path.substr(1 + itr);
    }

    return fanName;
}

/**
 * @function produces subtree paths whose names match fan token names.
 * @param[in] path - D-Bus path to obtain subtree from
 * @param[in] iface - interface to obtain subTreePaths from
 * @param[in] fans - label matching tokens to filter by
 * @param[in] shortPath - flag to shorten fan token
 * @return map of paths by fan name
 */

std::map<std::string, std::vector<std::string>> getPathsFromIface(
    const std::string& path, const std::string& iface,
    const std::vector<std::string>& fans, bool shortPath = false)
{
    std::map<std::string, std::vector<std::string>> dest;

    for (auto& path :
         SDBusPlus::getSubTreePathsRaw(SDBusPlus::getBus(), path, iface, 0))
    {
        for (auto& fan : fans)
        {
            if (shortPath)
            {
                if (fan == justFanName(path))
                {
                    dest[fan].push_back(path);
                }
            }
            else if (std::string::npos != path.find(fan + "_"))
            {
                dest[fan].push_back(path);
            }
        }
    }

    return dest;
}

/**
 * @function consolidated function to load dbus paths and fan names
 */
auto loadDBusData()
{
    auto& bus{SDBusPlus::getBus()};

    std::vector<std::string> fanNames;

    // paths by D-bus interface,fan name
    std::map<std::string, std::map<std::string, std::vector<std::string>>>
        pathMap;

    std::string method("RPM");

    std::map<const std::string, const std::string> interfaces{
        {"FanSpeed", "xyz.openbmc_project.Control.FanSpeed"},
        {"FanPwm", "xyz.openbmc_project.Control.FanPwm"},
        {"SensorValue", "xyz.openbmc_project.Sensor.Value"},
        {"Item", "xyz.openbmc_project.Inventory.Item"},
        {"OpStatus", "xyz.openbmc_project.State.Decorator.OperationalStatus"}};

    std::map<const std::string, const std::string> paths{
        {"motherboard",
         "/xyz/openbmc_project/inventory/system/chassis/motherboard"},
        {"tach", "/xyz/openbmc_project/sensors/fan_tach"}};

    // build a list of all fans
    for (auto& path : SDBusPlus::getSubTreePathsRaw(bus, paths["tach"],
                                                    interfaces["FanSpeed"], 0))
    {
        // special case where we build the list of fans
        auto fan = justFanName(path);
        fan = fan.substr(0, fan.rfind("_"));
        fanNames.push_back(fan);
    }

    // retry with PWM mode if none found
    if (0 == fanNames.size())
    {
        method = "PWM";

        for (auto& path : SDBusPlus::getSubTreePathsRaw(
                 bus, paths["tach"], interfaces["FanPwm"], 0))
        {
            // special case where we build the list of fans
            auto fan = justFanName(path);
            fan = fan.substr(0, fan.rfind("_"));
            fanNames.push_back(fan);
        }
    }

    // load tach sensor paths for each fan
    pathMap["tach"] =
        getPathsFromIface(paths["tach"], interfaces["SensorValue"], fanNames);

    // load inventory Item data for each fan
    pathMap["inventory"] = getPathsFromIface(
        paths["motherboard"], interfaces["Item"], fanNames, true);

    // load operational status data for each fan
    pathMap["opstatus"] = getPathsFromIface(
        paths["motherboard"], interfaces["OpStatus"], fanNames, true);

    return std::make_tuple(fanNames, pathMap, interfaces, method);
}

/**
 * @function gets the states of phosphor-fanctl. equivalent to
 * "systemctl status phosphor-fan-control@0"
 * @return a list of several (sub)states of fanctl (loaded,
 * active, running) as well as D-Bus properties representing
 * BMC states (bmc state,chassis power state, host state)
 */

std::array<std::string, 6> getStates()
{
    using DBusTuple =
        std::tuple<std::string, std::string, std::string, std::string,
                   std::string, std::string, sdbusplus::message::object_path,
                   uint32_t, std::string, sdbusplus::message::object_path>;

    std::array<std::string, 6> ret;

    std::vector<std::string> services{phosphorServiceName};

    try
    {
        auto fields{SDBusPlus::callMethodAndRead<std::vector<DBusTuple>>(
            systemdService, systemdPath, systemdMgrIface, "ListUnitsByNames",
            services)};

        if (fields.size() > 0)
        {
            ret[0] = std::get<2>(fields[0]);
            ret[1] = std::get<3>(fields[0]);
            ret[2] = std::get<4>(fields[0]);
        }
        else
        {
            std::cout << "No units found for systemd service: " << services[0]
                      << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failure retrieving phosphor-fan-control states: "
                  << e.what() << std::endl;
    }

    std::string path("/xyz/openbmc_project/state/bmc0");
    std::string iface("xyz.openbmc_project.State.BMC");
    ret[3] =
        SDBusPlus::getProperty<std::string>(path, iface, "CurrentBMCState");

    path = "/xyz/openbmc_project/state/chassis0";
    iface = "xyz.openbmc_project.State.Chassis";
    ret[4] =
        SDBusPlus::getProperty<std::string>(path, iface, "CurrentPowerState");

    path = "/xyz/openbmc_project/state/host0";
    iface = "xyz.openbmc_project.State.Host";
    ret[5] =
        SDBusPlus::getProperty<std::string>(path, iface, "CurrentHostState");

    return ret;
}

/**
 * @function helper to determine interface type from a given control method
 */
std::string ifaceTypeFromMethod(const std::string& method)
{
    return (method == "RPM" ? "FanSpeed" : "FanPwm");
}

/**
 * @function performs the "status" command from the cmdline.
 * get states and sensor data and output to the console
 */
void status()
{
    using std::cout;
    using std::endl;
    using std::setw;

    auto busData = loadDBusData();
    auto& method = std::get<METHOD>(busData);

    std::string property;

    // get the state,substate of fan-control and obmc
    auto states = getStates();

    // print the header
    cout << "Fan Control Service State   : " << states[0] << ", " << states[1]
         << "(" << states[2] << ")" << endl;
    cout << endl;
    cout << "CurrentBMCState     : " << states[3] << endl;
    cout << "CurrentPowerState   : " << states[4] << endl;
    cout << "CurrentHostState    : " << states[5] << endl;
    cout << endl;
    cout << "FAN       "
         << "TARGET(" << method << ")     FEEDBACKS(RPM)   PRESENT"
         << "   FUNCTIONAL" << endl;
    cout << "==============================================================="
         << endl;

    auto& fanNames{std::get<FAN_NAMES>(busData)};
    auto& pathMap{std::get<PATH_MAP>(busData)};
    auto& interfaces{std::get<IFACES>(busData)};

    for (auto& fan : fanNames)
    {
        cout << setw(8) << std::left << fan << std::right << setw(13);

        // get the target RPM
        property = "Target";
        cout << SDBusPlus::getProperty<uint64_t>(
                    pathMap["tach"][fan][0],
                    interfaces[ifaceTypeFromMethod(method)], property)
             << setw(19);

        // get the sensor RPM
        property = "Value";

        std::ostringstream output;
        int numRotors = pathMap["tach"][fan].size();
        // print tach readings for each rotor
        for (auto& path : pathMap["tach"][fan])
        {
            output << SDBusPlus::getProperty<double>(
                path, interfaces["SensorValue"], property);

            // dont print slash on last rotor
            if (--numRotors)
                output << "/";
        }
        cout << output.str() << setw(10);

        // print the Present property
        property = "Present";
        auto itFan = pathMap["inventory"].find(fan);
        if (itFan != pathMap["inventory"].end())
        {
            for (auto& path : itFan->second)
            {
                try
                {
                    cout << std::boolalpha
                         << SDBusPlus::getProperty<bool>(
                                path, interfaces["Item"], property);
                }
                catch (const phosphor::fan::util::DBusError&)
                {
                    cout << "Unknown";
                }
            }
        }
        else
        {
            cout << "Unknown";
        }

        cout << setw(13);

        // and the functional property
        property = "Functional";
        itFan = pathMap["opstatus"].find(fan);
        if (itFan != pathMap["opstatus"].end())
        {
            for (auto& path : itFan->second)
            {
                try
                {
                    cout << std::boolalpha
                         << SDBusPlus::getProperty<bool>(
                                path, interfaces["OpStatus"], property);
                }
                catch (const phosphor::fan::util::DBusError&)
                {
                    cout << "Unknown";
                }
            }
        }
        else
        {
            cout << "Unknown";
        }

        cout << endl;
    }
}

/**
 * @function print target RPM/PWM and tach readings from each fan
 */
void get()
{
    using std::cout;
    using std::endl;
    using std::setw;

    auto busData = loadDBusData();

    auto& fanNames{std::get<FAN_NAMES>(busData)};
    auto& pathMap{std::get<PATH_MAP>(busData)};
    auto& interfaces{std::get<IFACES>(busData)};
    auto& method = std::get<METHOD>(busData);

    std::string property;

    // print the header
    cout << "TARGET SENSOR" << setw(11) << "TARGET(" << method
         << ")   FEEDBACK SENSOR    FEEDBACK(RPM)" << endl;
    cout << "==============================================================="
         << endl;

    for (auto& fan : fanNames)
    {
        if (pathMap["tach"][fan].size() == 0)
            continue;
        // print just the sensor name
        auto shortPath = pathMap["tach"][fan][0];
        shortPath = justFanName(shortPath);
        cout << setw(13) << std::left << shortPath << std::right << setw(15);

        // print its target RPM/PWM
        property = "Target";
        cout << SDBusPlus::getProperty<uint64_t>(
            pathMap["tach"][fan][0], interfaces[ifaceTypeFromMethod(method)],
            property);

        // print readings for each rotor
        property = "Value";

        auto indent = 0;
        for (auto& path : pathMap["tach"][fan])
        {
            cout << setw(18 + indent) << justFanName(path) << setw(17)
                 << SDBusPlus::getProperty<double>(
                        path, interfaces["SensorValue"], property)
                 << endl;

            if (0 == indent)
                indent = 28;
        }
    }
}

/**
 * @function set fan[s] to a target RPM
 */
void set(uint64_t target, std::vector<std::string>& fanList)
{
    auto busData = loadDBusData();
    auto& bus{SDBusPlus::getBus()};
    auto& pathMap{std::get<PATH_MAP>(busData)};
    auto& interfaces{std::get<IFACES>(busData)};
    auto& method = std::get<METHOD>(busData);

    std::string ifaceType(method == "RPM" ? "FanSpeed" : "FanPwm");

    // stop the fan-control service
    SDBusPlus::callMethodAndRead<sdbusplus::message::object_path>(
        systemdService, systemdPath, systemdMgrIface, "StopUnit",
        phosphorServiceName, "replace");

    if (fanList.size() == 0)
    {
        fanList = std::get<FAN_NAMES>(busData);
    }

    for (auto& fan : fanList)
    {
        try
        {
            auto paths(pathMap["tach"].find(fan));

            if (pathMap["tach"].end() == paths)
            {
                // try again, maybe it was a sensor name instead of a fan name
                for (const auto& [fanName, sensors] : pathMap["tach"])
                {
                    for (const auto& path : sensors)
                    {
                        std::string sensor(path.substr(path.rfind("/")));

                        if (sensor.size() > 0)
                        {
                            sensor = sensor.substr(1);

                            if (sensor == fan)
                            {
                                paths = pathMap["tach"].find(fanName);

                                break;
                            }
                        }
                    }
                }
            }

            if (pathMap["tach"].end() == paths)
            {
                std::cout << "Could not find tach path for fan: " << fan
                          << std::endl;
                continue;
            }

            // set the target RPM
            SDBusPlus::setProperty<uint64_t>(bus, paths->second[0],
                                             interfaces[ifaceType], "Target",
                                             std::move(target));
        }
        catch (const phosphor::fan::util::DBusPropertyError& e)
        {
            std::cerr << "Cannot set target rpm for " << fan
                      << " caught D-Bus exception: " << e.what() << std::endl;
        }
    }
}

/**
 * @function restart fan-control to allow it to manage fan speeds
 */
void resume()
{
    try
    {
        auto retval =
            SDBusPlus::callMethodAndRead<sdbusplus::message::object_path>(
                systemdService, systemdPath, systemdMgrIface, "StartUnit",
                phosphorServiceName, "replace");
    }
    catch (const phosphor::fan::util::DBusMethodError& e)
    {
        std::cerr << "Unable to start fan control: " << e.what() << std::endl;
    }
}

/**
 * @function force reload of control files by sending HUP signal
 */
void reload()
{
    try
    {
        SDBusPlus::callMethod(systemdService, systemdPath, systemdMgrIface,
                              "KillUnit", phosphorServiceName, "main", SIGHUP);
    }
    catch (const phosphor::fan::util::DBusPropertyError& e)
    {
        std::cerr << "Unable to reload configuration files: " << e.what()
                  << std::endl;
    }
}

/**
 * @function dump debug data
 */
void dumpFanControl()
{
    namespace fs = std::filesystem;

    try
    {
        // delete existing file
        if (fs::exists(dumpFile))
        {
            std::filesystem::remove(dumpFile);
        }

        SDBusPlus::callMethod(systemdService, systemdPath, systemdMgrIface,
                              "KillUnit", phosphorServiceName, "main", SIGUSR1);

        bool done = false;
        size_t tries = 0;
        const size_t maxTries = 30;

        do
        {
            // wait for file to be detected
            sleep(1);

            if (fs::exists(dumpFile))
            {
                try
                {
                    auto unused{nlohmann::json::parse(std::ifstream{dumpFile})};
                    done = true;
                }
                catch (...)
                {}
            }

            if (++tries > maxTries)
            {
                std::cerr << "Timed out waiting for fan control dump.\n";
                return;
            }
        } while (!done);

        std::cout << "Fan control dump written to: " << dumpFile << std::endl;
    }
    catch (const phosphor::fan::util::DBusPropertyError& e)
    {
        std::cerr << "Unable to dump fan control: " << e.what() << std::endl;
    }
}

/**
 * @function Query items in the dump file
 */
void queryDumpFile(const DumpQuery& dq)
{
    nlohmann::json output;
    std::ifstream file{dumpFile};

    if (!file.good())
    {
        std::cerr << "Unable to open dump file, please run 'fanctl dump'.\n";
        return;
    }

    auto dumpData = nlohmann::json::parse(file);

    if (!dumpData.contains(dq.section))
    {
        std::cerr << "Error: Dump file does not contain " << dq.section
                  << " section"
                  << "\n";
        return;
    }

    const auto& section = dumpData.at(dq.section);

    if (section.is_array())
    {
        for (const auto& entry : section)
        {
            if (!entry.is_string() || dq.name.empty() ||
                (entry.get<std::string>().find(dq.name) != std::string::npos))
            {
                output[dq.section].push_back(entry);
            }
        }
        std::cout << std::setw(4) << output << "\n";
        return;
    }

    for (const auto& [key1, values1] : section.items())
    {
        if (dq.name.empty() || (key1.find(dq.name) != std::string::npos))
        {
            // If no properties specified, print the whole JSON value
            if (dq.properties.empty())
            {
                output[key1] = values1;
                continue;
            }

            // Look for properties both one and two levels down.
            // Future improvement: Use recursion.
            for (const auto& [key2, values2] : values1.items())
            {
                for (const auto& prop : dq.properties)
                {
                    if (prop == key2)
                    {
                        output[key1][prop] = values2;
                    }
                }

                for (const auto& [key3, values3] : values2.items())
                {
                    for (const auto& prop : dq.properties)
                    {
                        if (prop == key3)
                        {
                            output[key1][prop] = values3;
                        }
                    }
                }
            }
        }
    }

    if (!output.empty())
    {
        std::cout << std::setw(4) << output << "\n";
    }
}

/**
 * @function Get the sensor type based on the sensor name
 *
 * @param sensor The sensor object path
 */
std::string getSensorType(const std::string& sensor)
{
    // Get type from /xyz/openbmc_project/sensors/<type>/<name>
    try
    {
        auto type =
            sensor.substr(std::string{"/xyz/openbmc_project/sensors/"}.size());
        return type.substr(0, type.find_first_of('/'));
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed extracting type from sensor " << sensor << ": "
                  << e.what() << "\n";
    }
    return std::string{};
}

/**
 * @function Print the sensors passed in
 *
 * @param sensors The sensors to print
 */
void printSensors(const std::vector<SensorOutput>& sensors)
{
    size_t maxNameSize = 0;

    std::ranges::for_each(sensors, [&maxNameSize](const auto& s) {
        maxNameSize = std::max(maxNameSize, s.name.size());
    });

    std::ranges::for_each(sensors, [maxNameSize](const auto& sensor) {
        auto nameField = sensor.name + ':';
        std::cout << std::left << std::setw(maxNameSize + 2) << nameField
                  << sensor.value;
        if (!sensor.functional)
        {
            std::cout << " (Functional=false)";
        }

        if (!sensor.available)
        {
            std::cout << " (Available=false)";
        }
        std::cout << "\n";
    });
}

/**
 * @function Extracts the sensor out of the GetManagedObjects output
 *           for the one object path passed in.
 *
 * @param object The GetManagedObjects output for a single object path
 * @param opts The sensor options
 * @param[out] sensors Filled in with the sensor data
 */
void extractSensorData(const auto& object, const SensorOpts& opts,
                       std::vector<SensorOutput>& sensors)
{
    auto it = object.second.find("xyz.openbmc_project.Sensor.Value");
    if (it == object.second.end())
    {
        return;
    }
    auto value = std::get<double>(it->second.at("Value"));

    // Use the full D-Bus path of the sensor for the name if verbose
    std::string name = object.first.str;
    name = name.substr(name.find_last_of('/') + 1);
    std::string printName = name;
    if (opts.verbose)
    {
        printName = object.first.str;
    }

    // Apply the name filter
    if (!opts.name.empty())
    {
        if (!name.contains(opts.name))
        {
            return;
        }
    }

    // Apply the type filter
    if (!opts.type.empty())
    {
        if (opts.type != getSensorType(object.first.str))
        {
            return;
        }
    }

    bool functional = true;
    it = object.second.find(
        "xyz.openbmc_project.State.Decorator.OperationalStatus");
    if (it != object.second.end())
    {
        functional = std::get<bool>(it->second.at("Functional"));
    }

    bool available = true;
    it = object.second.find("xyz.openbmc_project.State.Decorator.Availability");
    if (it != object.second.end())
    {
        available = std::get<bool>(it->second.at("Available"));
    }

    sensors.emplace_back(printName, value, functional, available);
}

/**
 * @function Call GetManagedObjects on all sensor object managers and then
 *           print the sensor values.
 *
 * @param sensorManagers map<service, path> of sensor ObjectManagers
 * @param opts The sensor options
 */
void readSensorsAndPrint(std::map<std::string, std::string>& sensorManagers,
                         const SensorOpts& opts)
{
    std::vector<SensorOutput> sensors;

    using PropertyVariantType =
        std::variant<bool, int32_t, int64_t, double, std::string>;

    std::ranges::for_each(sensorManagers, [&opts, &sensors](const auto& entry) {
        auto values = SDBusPlus::getManagedObjects<PropertyVariantType>(
            SDBusPlus::getBus(), entry.first, entry.second);

        // Pull out the sensor details
        std::ranges::for_each(values, [&opts, &sensors](const auto& sensor) {
            extractSensorData(sensor, opts, sensors);
        });
    });

    std::ranges::sort(sensors, [](const auto& left, const auto& right) {
        return left.name < right.name;
    });

    printSensors(sensors);
}

/**
 * @function Prints sensor values
 *
 * @param opts The sensor options
 */
void displaySensors(const SensorOpts& opts)
{
    // Find the services that provide sensors
    auto sensorObjects = SDBusPlus::getSubTreeRaw(
        SDBusPlus::getBus(), "/", "xyz.openbmc_project.Sensor.Value", 0);

    std::set<std::string> sensorServices;

    std::ranges::for_each(sensorObjects, [&sensorServices](const auto& object) {
        sensorServices.insert(object.second.begin()->first);
    });

    // Find the ObjectManagers for those services
    auto objectManagers = SDBusPlus::getSubTreeRaw(
        SDBusPlus::getBus(), "/", "org.freedesktop.DBus.ObjectManager", 0);

    std::map<std::string, std::string> managers;

    std::ranges::for_each(
        objectManagers, [&sensorServices, &managers](const auto& object) {
            // Check every service on this path
            std::ranges::for_each(
                object.second, [&managers, path = object.first,
                                &sensorServices](const auto& entry) {
                    // Check if this service provides sensors
                    if (std::ranges::contains(sensorServices, entry.first))
                    {
                        managers[entry.first] = path;
                    }
                });
        });

    readSensorsAndPrint(managers, opts);
}

/**
 * @function setup the CLI object to accept all options
 */
void initCLI(CLI::App& app, uint64_t& target, std::vector<std::string>& fanList,
             [[maybe_unused]] DumpQuery& dq, SensorOpts& sensorOpts)
{
    app.set_help_flag("-h,--help", "Print this help page and exit.");

    // App requires only 1 subcommand to be given
    app.require_subcommand(1);

    // This represents the command given
    auto commands = app.add_option_group("Commands");

    // status method
    std::string strHelp("Prints fan target/tach readings, present/functional "
                        "states, and fan-monitor/BMC/Power service status");

    auto cmdStatus = commands->add_subcommand("status", strHelp);
    cmdStatus->set_help_flag("-h, --help", strHelp);
    cmdStatus->require_option(0);

    // get method
    strHelp = "Get the current fan target and feedback speeds for all rotors";
    auto cmdGet = commands->add_subcommand("get", strHelp);
    cmdGet->set_help_flag("-h, --help", strHelp);
    cmdGet->require_option(0);

    // set method
    strHelp = "Set target (all rotors) for one-or-more fans";
    auto cmdSet = commands->add_subcommand("set", strHelp);
    strHelp = R"(set <TARGET> [TARGET SENSOR(S)]
  <TARGET>
      - RPM/PWM target to set the fans
[TARGET SENSOR LIST]
- list of target sensors to set)";
    cmdSet->set_help_flag("-h, --help", strHelp);
    cmdSet->add_option("target", target, "RPM/PWM target to set the fans");
    cmdSet->add_option(
        "fan list", fanList,
        "[optional] list of 1+ fans to set target RPM/PWM (default: all)");
    cmdSet->require_option();

#ifdef CONTROL_USE_JSON
    strHelp = "Reload phosphor-fan configuration files";
    auto cmdReload = commands->add_subcommand("reload", strHelp);
    cmdReload->set_help_flag("-h, --help", strHelp);
    cmdReload->require_option(0);
#endif

    strHelp = "Resume running phosphor-fan-control";
    auto cmdResume = commands->add_subcommand("resume", strHelp);
    cmdResume->set_help_flag("-h, --help", strHelp);
    cmdResume->require_option(0);

    // Dump method
    auto cmdDump = commands->add_subcommand("dump", "Dump debug data");
    cmdDump->set_help_flag("-h, --help", "Dump debug data");
    cmdDump->require_option(0);

#ifdef CONTROL_USE_JSON
    // Query dump
    auto cmdDumpQuery =
        commands->add_subcommand("query_dump", "Query the dump file");

    cmdDumpQuery->set_help_flag("-h, --help", "Query the dump file");
    cmdDumpQuery
        ->add_option("-s, --section", dq.section, "Dump file section name")
        ->required();
    cmdDumpQuery->add_option("-n, --name", dq.name,
                             "Optional dump file entry name (or substring)");
    cmdDumpQuery->add_option("-p, --properties", dq.properties,
                             "Optional list of dump file property names");
    cmdDumpQuery->add_flag("-d, --dump", dq.dump,
                           "Force a dump before the query");
#endif

    auto cmdSensors =
        commands->add_subcommand("sensors", "Retrieve sensor values");
    cmdSensors->set_help_flag("-h, --help", "Retrieve sensor values");
    cmdSensors->add_option(
        "-t, --type", sensorOpts.type,
        "Only show sensors of this type (i.e. 'temperature'). Optional");
    cmdSensors->add_option(
        "-n, --name", sensorOpts.name,
        "Only show sensors with this string in the name. Optional");
    cmdSensors->add_flag("-v, --verbose", sensorOpts.verbose,
                         "Verbose: Use sensor object path for the name");
}

/**
 * @function main entry point for the application
 */
int main(int argc, char* argv[])
{
    auto rc = 0;
    uint64_t target{0U};
    std::vector<std::string> fanList;
    DumpQuery dq;
    SensorOpts sensorOpts;

    try
    {
        CLI::App app{"Manually control, get fan tachs, view status, and resume "
                     "automatic control of all fans within a chassis. Full "
                     "documentation can be found at the readme:\n"
                     "https://github.com/openbmc/phosphor-fan-presence/tree/"
                     "master/docs/control/fanctl"};

        initCLI(app, target, fanList, dq, sensorOpts);

        CLI11_PARSE(app, argc, argv);

        if (app.got_subcommand("get"))
        {
            get();
        }
        else if (app.got_subcommand("set"))
        {
            set(target, fanList);
        }
#ifdef CONTROL_USE_JSON
        else if (app.got_subcommand("reload"))
        {
            reload();
        }
#endif
        else if (app.got_subcommand("resume"))
        {
            resume();
        }
        else if (app.got_subcommand("status"))
        {
            status();
        }
        else if (app.got_subcommand("dump"))
        {
#ifdef CONTROL_USE_JSON
            dumpFanControl();
#else
            std::ofstream(dumpFile)
                << "{\n\"msg\":   \"Unable to create dump on "
                   "non-JSON config based system\"\n}";
#endif
        }
#ifdef CONTROL_USE_JSON
        else if (app.got_subcommand("query_dump"))
        {
            if (dq.dump)
            {
                dumpFanControl();
            }
            queryDumpFile(dq);
        }
#endif
        else if (app.got_subcommand("sensors"))
        {
            displaySensors(sensorOpts);
        }
    }
    catch (const std::exception& e)
    {
        rc = -1;
        std::cerr << argv[0] << " failed: " << e.what() << std::endl;
    }

    return rc;
}
