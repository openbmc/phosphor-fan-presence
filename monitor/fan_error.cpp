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
#include "fan_error.hpp"

#include "logging.hpp"
#include "sdbusplus.hpp"

#include <nlohmann/json.hpp>
#include <xyz/openbmc_project/Logging/Create/server.hpp>

#include <filesystem>

namespace phosphor::fan::monitor
{

using FFDCFormat =
    sdbusplus::xyz::openbmc_project::Logging::server::Create::FFDCFormat;
using FFDCFiles = std::vector<
    std::tuple<FFDCFormat, uint8_t, uint8_t, sdbusplus::message::unix_fd>>;
using json = nlohmann::json;

const auto loggingService = "xyz.openbmc_project.Logging";
const auto loggingPath = "/xyz/openbmc_project/logging";
const auto loggingCreateIface = "xyz.openbmc_project.Logging.Create";

namespace fs = std::filesystem;
using namespace phosphor::fan::util;

FFDCFile::FFDCFile(const fs::path& name) :
    _fd(open(name.c_str(), O_RDONLY)), _name(name)
{
    if (_fd() == -1)
    {
        auto e = errno;
        getLogger().log(fmt::format("Could not open FFDC file {}. errno {}",
                                    _name.string(), e));
    }
}

void FanError::commit(const json& jsonFFDC, bool isPowerOffError)
{
    FFDCFiles ffdc;
    auto ad = getAdditionalData(isPowerOffError);

    // Add the Logger contents as FFDC
    auto logFile = makeLogFFDCFile();
    if (logFile && (logFile->fd() != -1))
    {
        ffdc.emplace_back(FFDCFormat::Text, 0x01, 0x01, logFile->fd());
    }

    // Add the passed in JSON as FFDC
    auto ffdcFile = makeJsonFFDCFile(jsonFFDC);
    if (ffdcFile && (ffdcFile->fd() != -1))
    {
        ffdc.emplace_back(FFDCFormat::JSON, 0x01, 0x01, ffdcFile->fd());
    }

    try
    {
        auto sev = _severity;

        // If this is a power off, change severity to Critical
        if (isPowerOffError)
        {
            using namespace sdbusplus::xyz::openbmc_project::Logging::server;
            sev = convertForMessage(Entry::Level::Critical);
        }
        SDBusPlus::callMethod(loggingService, loggingPath, loggingCreateIface,
                              "CreateWithFFDCFiles", _errorName, sev, ad, ffdc);
    }
    catch (const DBusError& e)
    {
        getLogger().log(
            fmt::format("Call to create a {} error for fan {} failed: {}",
                        _errorName, _fanName, e.what()),
            Logger::error);
    }
}

std::map<std::string, std::string>
    FanError::getAdditionalData(bool isPowerOffError)
{
    std::map<std::string, std::string> ad;

    ad.emplace("_PID", std::to_string(getpid()));
    ad.emplace("CALLOUT_INVENTORY_PATH", _fanName);

    if (!_sensorName.empty())
    {
        ad.emplace("FAN_SENSOR", _sensorName);
    }

    // If this is a power off, specify that it's a power
    // fault and a system termination.  This is used by some
    // implementations for service reasons.
    if (isPowerOffError)
    {
        ad.emplace("POWER_THERMAL_CRITICAL_FAULT", "TRUE");
        ad.emplace("SEVERITY_DETAIL", "SYSTEM_TERM");
    }

    return ad;
}

std::unique_ptr<FFDCFile> FanError::makeLogFFDCFile()
{
    try
    {
        auto logFile = getLogger().saveToTempFile();
        return std::make_unique<FFDCFile>(logFile);
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(
            fmt::format("Could not save log contents in FFDC. Error msg: {}",
                        e.what())
                .c_str());
    }
    return nullptr;
}

std::unique_ptr<FFDCFile> FanError::makeJsonFFDCFile(const json& ffdcData)
{
    char tmpFile[] = "/tmp/fanffdc.XXXXXX";
    auto fd = mkstemp(tmpFile);
    if (fd != -1)
    {
        auto jsonString = ffdcData.dump();

        auto rc = write(fd, jsonString.data(), jsonString.size());
        close(fd);
        if (rc != -1)
        {
            fs::path jsonFile{tmpFile};
            return std::make_unique<FFDCFile>(jsonFile);
        }
        else
        {
            getLogger().log("Failed call to write JSON FFDC file");
        }
    }
    else
    {
        auto e = errno;
        getLogger().log(fmt::format("Failed called to mkstemp, errno = {}", e),
                        Logger::error);
    }
    return nullptr;
}

} // namespace phosphor::fan::monitor
