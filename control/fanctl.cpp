/**
 * Copyright © 2021 IBM Corporation
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

#include "sdbusplus.hpp"

#include <CLI/CLI.hpp>
#include <sdbusplus/bus.hpp>

#include <iomanip>
#include <iostream>

using SDBusPlus = phosphor::fan::util::SDBusPlus;

constexpr auto phosphorServiceName = "phosphor-fan-control@0.service";
constexpr auto systemdMgrIface = "org.freedesktop.systemd1.Manager";
constexpr auto systemdPath = "/org/freedesktop/systemd1";
constexpr auto systemdService = "org.freedesktop.systemd1";

enum
{
    FAN_NAMES = 0,
    PATH_MAP = 1,
    IFACES = 2,
    METHOD = 3
};

/**
 * @function extracts fan name from dbus path string (last token where
 * delimiter is the / character), with proper bounds checking.
 * @param[in] path - D-Bus path
 * @return just the fan name.
 */

std::string justFanName(std::string const& path)
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

std::map<std::string, std::vector<std::string>>
    getPathsFromIface(const std::string& path, const std::string& iface,
                      const std::vector<std::string>& fans,
                      bool shortPath = false)
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
    cout << "CurrentBMCState     : " << states[3] << endl;
    cout << "CurrentPowerState   : " << states[4] << endl;
    cout << "CurrentHostState    : " << states[5] << endl;
    cout << endl;
    cout << " FAN        "
         << "TARGET(" << method << ")  FEEDBACKS(RPMS)   PRESENT"
         << "     FUNCTIONAL" << endl;
    cout << "==============================================================="
         << endl;

    auto& fanNames{std::get<FAN_NAMES>(busData)};
    auto& pathMap{std::get<PATH_MAP>(busData)};
    auto& interfaces{std::get<IFACES>(busData)};

    for (auto& fan : fanNames)
    {
        cout << " " << fan << setw(18);

        // get the target RPM
        property = "Target";
        cout << SDBusPlus::getProperty<uint64_t>(
                    pathMap["tach"][fan][0],
                    interfaces[ifaceTypeFromMethod(method)], property)
             << setw(11);

        // get the sensor RPM
        property = "Value";

        int numRotors = pathMap["tach"][fan].size();
        // print tach readings for each rotor
        for (auto& path : pathMap["tach"][fan])
        {
            cout << SDBusPlus::getProperty<double>(
                path, interfaces["SensorValue"], property);

            // dont print slash on last rotor
            if (--numRotors)
                cout << "/";
        }
        cout << setw(10);

        // print the Present property
        property = "Present";
        std::string val;
        for (auto& path : pathMap["inventory"][fan])
        {
            try
            {
                if (SDBusPlus::getProperty<bool>(path, interfaces["Item"],
                                                 property))
                {
                    val = "true";
                }
                else
                {
                    val = "false";
                }
            }
            catch (phosphor::fan::util::DBusPropertyError&)
            {
                val = "Unknown";
            }
            cout << val;
        }

        cout << setw(13);

        // and the functional property
        property = "Functional";
        for (auto& path : pathMap["opstatus"][fan])
        {
            try
            {
                if (SDBusPlus::getProperty<bool>(path, interfaces["OpStatus"],
                                                 property))
                {
                    val = "true";
                }
                else
                {
                    val = "false";
                }
            }
            catch (phosphor::fan::util::DBusPropertyError&)
            {
                val = "Unknown";
            }
            cout << val;
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
         << ")   FEEDBACK SENSOR   ";
    cout << "FEEDBACK(" << method << ")" << endl;
    cout << "==============================================================="
         << endl;

    for (auto& fan : fanNames)
    {
        if (pathMap["tach"][fan].size() == 0)
            continue;
        // print just the sensor name
        auto shortPath = pathMap["tach"][fan][0];
        shortPath = justFanName(shortPath);
        cout << shortPath << setw(22);

        // print its target RPM/PWM
        property = "Target";
        cout << SDBusPlus::getProperty<uint64_t>(
                    pathMap["tach"][fan][0],
                    interfaces[ifaceTypeFromMethod(method)], property)
             << setw(12) << " ";

        // print readings for each rotor
        property = "Value";

        auto indent = 0U;
        for (auto& path : pathMap["tach"][fan])
        {
            cout << setw(indent);
            cout << justFanName(path) << setw(17)
                 << SDBusPlus::getProperty<double>(
                        path, interfaces["SensorValue"], property)
                 << endl;

            if (0 == indent)
                indent = 46;
        }
    }
}

/**
 * @function set fan[s] to a target RPM
 */
void set(uint64_t target, std::vector<std::string> fanList)
{
    auto busData = loadDBusData();

    auto& bus{SDBusPlus::getBus()};
    auto& pathMap{std::get<PATH_MAP>(busData)};
    auto& interfaces{std::get<IFACES>(busData)};
    auto& method = std::get<METHOD>(busData);

    std::string ifaceType(method == "RPM" ? "FanSpeed" : "FanPwm");

    // stop the fan-control service
    auto retval = SDBusPlus::callMethodAndRead<sdbusplus::message::object_path>(
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
            auto paths(pathMap["speed"].find(fan));

            if (pathMap["speed"].end() == paths)
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
        catch (phosphor::fan::util::DBusPropertyError& e)
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
    catch (phosphor::fan::util::DBusMethodError& e)
    {
        std::cerr << "Unable to start fan control: " << e.what() << std::endl;
    }
}

/**
 * @function main entry point for the application
 */
int main(int argc, char* argv[])
{
    auto rc = 0;
    uint64_t target{0U};
    std::vector<std::string> fanList;

    try
    {
        CLI::App app{R"(Manually control, get fan tachs, view status, and resume
             automatic control of all fans within a chassis.)"};

        app.set_help_flag("-h,--help", "Print this help page and exit.");

        // App requires only 1 subcommand to be given
        app.require_subcommand(1);

        // This represents the command given
        auto commands = app.add_option_group("Commands");

        // status method
        auto cmdStatus = commands->add_subcommand(
            "status",
            "Get the fan tach targets/values and fan-control service status");
        cmdStatus->set_help_flag(
            "-h, --help", "Prints fan target/tach readings, present/functional "
                          "states, and fan-monitor/BMC/Power service status");
        cmdStatus->require_option(0);

        // get method
        auto cmdGet = commands->add_subcommand(
            "get",
            "Get the current fan target and feedback speeds for all rotors");
        cmdGet->set_help_flag(
            "-h, --help",
            "Get the current fan target and feedback speeds for all rotors");
        cmdGet->require_option(0);

        // set method
        auto cmdSet = commands->add_subcommand(
            "set", "Set fan(s) target speed for all rotors");
        cmdSet->add_option("target", target, "RPM/PWM target to set the fans");
        cmdSet->add_option("fan list", fanList,
                           "[optional] list of fans to set target RPM");

        auto setHelp(R"(set <TARGET> [\"TARGET SENSOR LIST\"]
      <TARGET>
          - RPM/PWM target to set the fans
[TARGET SENSOR LIST]
  - list of target sensors to set)");

        cmdSet->set_help_flag("-h, --help", setHelp);
        cmdSet->require_option(2);

        CLI11_PARSE(app, argc, argv);

        if (app.got_subcommand("status"))
        {
            status();
        }
        else if (app.got_subcommand("set"))
        {
            set(target, fanList);
        }
        else if (app.got_subcommand("get"))
        {
            get();
        }
        else if (app.got_subcommand("resume"))
        {
            resume();
        }
    }
    catch (const std::exception& e)
    {
        rc = -1;
        std::cerr << argv[0] << " failed: " << e.what() << std::endl;
    }

    return rc;
}
