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

#include <fmt/format.h>

#include <CLI/CLI.hpp>
#include <sdbusplus/bus.hpp>

#include <iomanip>
#include <iostream>

using SDBusPlus = phosphor::fan::util::SDBusPlus;

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

    std::string tmethod("RPM"), fmethod("RPMS");

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

    // retry if none found
    if (0 == fanNames.size())
    {
        tmethod = "PWM";
        fmethod = "PWM";

        // TODO: test this function
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

    // load speed sensor paths for each fan
    pathMap["speed"] = pathMap["tach"];

    // load inventory Item data for each fan
    pathMap["inventory"] = getPathsFromIface(
        paths["motherboard"], interfaces["Item"], fanNames, true);

    // load operational status data for each fan
    pathMap["opstatus"] = getPathsFromIface(
        paths["motherboard"], interfaces["OpStatus"], fanNames, true);

    return std::make_tuple(fanNames, pathMap, interfaces, tmethod, fmethod);
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

    // constexpr auto phosphorServiceName = "phosphor-fan-control@0.service";
    constexpr auto systemdMgrIface = "org.freedesktop.systemd1.Manager";
    constexpr auto systemdPath = "/org/freedesktop/systemd1";
    constexpr auto systemdService = "org.freedesktop.systemd1";

    std::array<std::string, 6> ret;

    std::vector<std::string> services{"phosphor-fan-control@0.service"};

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
        std::cout << "caught exception: " << e.what() << std::endl;
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
 * @function performs the "status" command from the cmdline.
 * get states and sensor data and output to the console
 */
void status()
{
    using std::cout;
    using std::endl;
    using std::setw;

    auto busData = loadDBusData();

    auto& tmethod = std::get<3>(busData);
    auto& fmethod = std::get<4>(busData);

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
         << "TARGET(" << tmethod << ")  FEEDBACKS(";
    cout << fmethod << ")   PRESENT"
         << "     FUNCTIONAL" << endl;
    cout << "==============================================================="
         << endl;

    auto& fanNames{std::get<0>(busData)};
    auto& pathMap{std::get<1>(busData)};
    auto& interfaces{std::get<2>(busData)};

    for (auto& fan : fanNames)
    {
        cout << " " << fan << setw(18);

        // get the target RPM
        property = "Target";
        cout << SDBusPlus::getProperty<uint64_t>(
                    pathMap["speed"][fan][0], interfaces["FanSpeed"], property)
             << setw(11);

        // get the sensor RPM (target/actual)
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
 * @function print usage information to the console
 */
void get()
{
    using std::cout;
    using std::endl;
    using std::setw;

    std::string property;

    auto busData = loadDBusData();

    auto& fanNames{std::get<0>(busData)};
    auto& pathMap{std::get<1>(busData)};
    auto& interfaces{std::get<2>(busData)};
    auto& tmethod = std::get<3>(busData);
    auto& fmethod = std::get<4>(busData);

    // print the header
    cout << "TARGET SENSOR" << setw(11) << "TARGET(" << tmethod
         << ")   FEEDBACK SENSOR   ";
    cout << "FEEDBACK(" << fmethod << ")" << endl;
    cout << "==============================================================="
         << endl;

    for (auto& fan : fanNames)
    {
        // print just the sensor name
        auto shortPath = pathMap["tach"][fan][0];
        shortPath = justFanName(shortPath);
        cout << shortPath << setw(22);

        // print its target RPM
        property = "Target";
        cout << SDBusPlus::getProperty<uint64_t>(
                    pathMap["speed"][fan][0], interfaces["FanSpeed"], property)
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
 * @function print usage information to the console
 */
void printHelp()
{
    std::cout << "NAME\n\
    fanctl - Manually control, get fan tachs, view status, and resume\n\
             automatic control of all fans within a chassis.\n\
SYNOPSIS\n\
    fanctl [OPTION]\n\
OPTIONS\n\
  status\n\
      - Get the full system status in regard to fans\n\
  get\n\
      - Get the current fan target and feedback speeds for all rotors\n\
  help\n\
      - Display this help and exit\n";
}

/**
 * @function main entry point for the application
 */
int main(int argc, char* argv[])
{
    std::string action("help");
    CLI::App app{"OpenBMC Fan Control App"};
    app.add_option("status", action, "display fan-control status");
    CLI11_PARSE(app, argc, argv);

    try
    {
        if ("status" == action)
        {
            status();
        }
        else if ("get" == action)
        {
            get();
        }
        else
        {
            printHelp();
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Fan control failed: " << e.what() << std::endl;
    }

    return 0;
}
