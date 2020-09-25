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
#include "error_reporter.hpp"

#include "logging.hpp"
#include "psensor.hpp"
#include "utility.hpp"

#include <fmt/format.h>
#include <unistd.h>

#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Logging/Create/server.hpp>
#include <xyz/openbmc_project/Logging/Entry/server.hpp>

namespace phosphor::fan::presence
{

using json = nlohmann::json;
using namespace phosphor::logging;
using namespace sdbusplus::bus::match;
using namespace std::literals::string_literals;
namespace fs = std::filesystem;

const auto itemIface = "xyz.openbmc_project.Inventory.Item"s;
const auto invPrefix = "/xyz/openbmc_project/inventory"s;
const auto loggingPath = "/xyz/openbmc_project/logging";
const auto loggingCreateIface = "xyz.openbmc_project.Logging.Create";

ErrorReporter::ErrorReporter(
    sdbusplus::bus::bus& bus, const json& jsonConf,
    const std::vector<
        std::tuple<Fan, std::vector<std::unique_ptr<PresenceSensor>>>>& fans) :
    _bus(bus),
    _event(sdeventplus::Event::get_default())
{
    loadConfig(jsonConf);

    // If different methods to check the power state are needed across the
    // various platforms, the method/class to use could be read out of JSON
    // or set with a compilation flag.
    _powerState = std::make_unique<PGoodState>(
        bus, std::bind(std::mem_fn(&ErrorReporter::powerStateChanged), this,
                       std::placeholders::_1));

    for (const auto& fan : fans)
    {
        auto path = invPrefix + std::get<1>(std::get<0>(fan));

        // Register for fan presence changes, get their initial states,
        // and create the fan missing timers for each fan.

        _matches.emplace_back(
            _bus, rules::propertiesChanged(path, itemIface),
            std::bind(std::mem_fn(&ErrorReporter::presenceChanged), this,
                      std::placeholders::_1));

        _fanStates.emplace(path, getPresence(std::get<0>(fan)));

        auto timer = std::make_unique<
            sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>>(
            _event,
            std::bind(std::mem_fn(&ErrorReporter::fanMissingTimerExpired), this,
                      path));

        _fanMissingTimers.emplace(path, std::move(timer));
    }

    // If power is already on, check for currently missing fans.
    if (_powerState->isPowerOn())
    {
        powerStateChanged(true);
    }
}

void ErrorReporter::loadConfig(const json& jsonConf)
{
    if (!jsonConf.contains("fan_missing_error_time"))
    {
        log<level::ERR>("Missing 'fan_missing_error_time' entry in JSON "
                        "'reporting' section");

        throw std::runtime_error("Missing fan missing time entry in JSON");
    }

    _fanMissingErrorTime = std::chrono::seconds{
        jsonConf.at("fan_missing_error_time").get<std::size_t>()};
}

void ErrorReporter::presenceChanged(sdbusplus::message::message& msg)
{
    bool present;
    auto fanPath = msg.get_path();
    std::string interface;
    std::map<std::string, std::variant<bool>> properties;

    msg.read(interface, properties);

    auto presentProp = properties.find("Present");
    if (presentProp != properties.end())
    {
        present = std::get<bool>(presentProp->second);
        if (_fanStates[fanPath] != present)
        {
            getLogger().log(fmt::format("Fan {} presence state change to {}",
                                        fanPath, present));

            _fanStates[fanPath] = present;
            checkFan(fanPath);
        }
    }
}

void ErrorReporter::checkFan(const std::string& fanPath)
{
    if (!_fanStates[fanPath])
    {
        // Fan is missing. If power is on, start the timer.
        // If power is off, stop a running timer.
        if (_powerState->isPowerOn())
        {
            _fanMissingTimers[fanPath]->restartOnce(_fanMissingErrorTime);
        }
        else if (_fanMissingTimers[fanPath]->isEnabled())
        {
            _fanMissingTimers[fanPath]->setEnabled(false);
        }
    }
    else
    {
        // Fan is present. Stop a running timer.
        if (_fanMissingTimers[fanPath]->isEnabled())
        {
            _fanMissingTimers[fanPath]->setEnabled(false);
        }
    }
}

void ErrorReporter::fanMissingTimerExpired(const std::string& fanPath)
{
    getLogger().log(
        fmt::format("Creating event log for missing fan {}", fanPath),
        Logger::error);

    std::map<std::string, std::string> additionalData;
    additionalData.emplace("_PID", std::to_string(getpid()));
    additionalData.emplace("CALLOUT_INVENTORY_PATH", fanPath);

    auto severity =
        sdbusplus::xyz::openbmc_project::Logging::server::convertForMessage(
            sdbusplus::xyz::openbmc_project::Logging::server::Entry::Level::
                Error);

    // Save our logs in JSON to a temp file and get the file descriptor
    // so it can be passed in as FFDC data.
    auto logFile = getLogger().saveToTempFile();
    util::FileDescriptor fd{-1};
    fd.open(logFile, O_RDONLY);

    std::vector<std::tuple<
        sdbusplus::xyz::openbmc_project::Logging::server::Create::FFDCFormat,
        uint8_t, uint8_t, sdbusplus::message::unix_fd>>
        ffdc;

    ffdc.emplace_back(sdbusplus::xyz::openbmc_project::Logging::server::Create::
                          FFDCFormat::JSON,
                      0x01, 0x01, fd());

    try
    {
        util::SDBusPlus::lookupAndCallMethod(
            loggingPath, loggingCreateIface, "CreateWithFFDCFiles",
            "xyz.openbmc_project.Fan.Error.Missing", severity, additionalData,
            ffdc);
    }
    catch (const util::DBusError& e)
    {
        getLogger().log(
            fmt::format(
                "Call to create an error log for missing fan {} failed: {}",
                fanPath, e.what()),
            Logger::error);
        fs::remove(logFile);
        throw;
    }

    fs::remove(logFile);
}

void ErrorReporter::powerStateChanged(bool powerState)
{
    if (powerState)
    {
        // If there are fans already missing, log it.
        auto missing = std::count_if(
            _fanStates.begin(), _fanStates.end(),
            [](const auto& fanState) { return fanState.second == false; });

        if (missing)
        {
            getLogger().log(
                fmt::format("At power on, there are {} missing fans", missing));
        }
    }

    std::for_each(
        _fanStates.begin(), _fanStates.end(),
        [this](const auto& fanState) { this->checkFan(fanState.first); });
}

} // namespace phosphor::fan::presence
