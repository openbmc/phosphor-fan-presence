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
#include "fan_error.hpp"

#include "logging.hpp"
#include "sdbusplus.hpp"

#include <systemd/sd-journal.h>

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

/**
 * @class JournalCloser
 *
 * Automatically closes the journal when the object goes out of scope.
 */
class JournalCloser
{
  public:
    // Specify which compiler-generated methods we want
    JournalCloser() = delete;
    JournalCloser(const JournalCloser&) = delete;
    JournalCloser(JournalCloser&&) = delete;
    JournalCloser& operator=(const JournalCloser&) = delete;
    JournalCloser& operator=(JournalCloser&&) = delete;

    explicit JournalCloser(sd_journal* journal) : journal{journal}
    {}

    ~JournalCloser()
    {
        sd_journal_close(journal);
    }

  private:
    sd_journal* journal{nullptr};
};

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

    // add the previous systemd journal entries as FFDC
    auto serviceFFDC = makeJsonFFDCFile(getJournalEntries(25));
    if (serviceFFDC && serviceFFDC->fd() != -1)
    {
        ffdc.emplace_back(FFDCFormat::JSON, 0x01, 0x01, serviceFFDC->fd());
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

    if (!_fanName.empty())
    {
        ad.emplace("CALLOUT_INVENTORY_PATH", _fanName);
    }

    if (!_sensorName.empty())
    {
        ad.emplace("FAN_SENSOR", _sensorName);
    }

    // If this is a power off, specify that it's a power
    // fault and a system termination.  This is used by some
    // implementations for service reasons.
    if (isPowerOffError)
    {
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

nlohmann::json FanError::getJournalEntries(int numLines) const
{
    // Sleep 100ms; otherwise recent journal entries sometimes not available
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(100ms);

    std::vector<std::string> entries;

    // Open the journal
    sd_journal* journal;
    int rc = sd_journal_open(&journal, SD_JOURNAL_LOCAL_ONLY);
    if (rc < 0)
    {
        // Build one line string containing field values
        entries.push_back("[Internal error: sd_journal_open(), rc=" +
                          std::string(strerror(rc)) + "]");
        return json(entries);
    }

    // Create object to automatically close journal
    JournalCloser closer{journal};

    std::string field{"SYSLOG_IDENTIFIER"};
    std::vector<std::string> executables{"systemd"};

    entries.reserve(2 * numLines);

    for (const auto& executable : executables)
    {
        // Add match so we only loop over entries with specified field value
        std::string match{field + '=' + executable};
        rc = sd_journal_add_match(journal, match.c_str(), 0);
        if (rc < 0)
        {
            // Build one line string containing field values
            entries.push_back("[Internal error: sd_journal_add_match(), rc=" +
                              std::string(strerror(rc)) + "]");

            break;
        }

        int count{1};

        std::string syslogID, pid, message, timeStamp;

        // Loop through journal entries from newest to oldest
        SD_JOURNAL_FOREACH_BACKWARDS(journal)
        {
            // Get relevant journal entry fields
            timeStamp = getTimeStamp(journal);
            syslogID = getFieldValue(journal, "SYSLOG_IDENTIFIER");
            pid = getFieldValue(journal, "_PID");
            message = getFieldValue(journal, "MESSAGE");

            // Build one line string containing field values
            entries.push_back(timeStamp + " " + syslogID + "[" + pid +
                              "]: " + message);

            // Stop after number of lines was read
            if (count++ >= numLines)
            {
                break;
            }
        }
    }

    // put the journal entries in chronological order
    std::reverse(entries.begin(), entries.end());

    return json(entries);
}

std::string FanError::getTimeStamp(sd_journal* journal) const
{
    // Get realtime (wallclock) timestamp of current journal entry.  The
    // timestamp is in microseconds since the epoch.
    uint64_t usec{0};
    int rc = sd_journal_get_realtime_usec(journal, &usec);
    if (rc < 0)
    {
        return "[Internal error: sd_journal_get_realtime_usec(), rc=" +
               std::string(strerror(rc)) + "]";
    }

    // Convert to number of seconds since the epoch
    time_t secs = usec / 1000000;

    // Convert seconds to tm struct required by strftime()
    struct tm* timeStruct = localtime(&secs);
    if (timeStruct == nullptr)
    {
        return "[Internal error: localtime() returned nullptr]";
    }

    // Convert tm struct into a date/time string
    char timeStamp[80];
    strftime(timeStamp, sizeof(timeStamp), "%b %d %H:%M:%S", timeStruct);

    return timeStamp;
}

std::string FanError::getFieldValue(sd_journal* journal,
                                    const std::string& field) const
{
    std::string value{};

    // Get field data from current journal entry
    const void* data{nullptr};
    size_t length{0};
    int rc = sd_journal_get_data(journal, field.c_str(), &data, &length);
    if (rc < 0)
    {
        if (-rc == ENOENT)
        {
            // Current entry does not include this field; return empty value
            return value;
        }
        else
        {
            return "[Internal error: sd_journal_get_data() rc=" +
                   std::string(strerror(rc)) + "]";
        }
    }

    // Get value from field data.  Field data in format "FIELD=value".
    std::string dataString{static_cast<const char*>(data), length};
    std::string::size_type pos = dataString.find('=');
    if ((pos != std::string::npos) && ((pos + 1) < dataString.size()))
    {
        // Value is substring after the '='
        value = dataString.substr(pos + 1);
    }

    return value;
}

} // namespace phosphor::fan::monitor
