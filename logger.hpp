#pragma once

#include "utility.hpp"

#include <unistd.h>

#include <nlohmann/json.hpp>
#include <phosphor-logging/lg2.hpp>

#include <cassert>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace phosphor::fan
{

/**
 * @class Logger
 *
 * A simple logging class that stores log messages in a vector along
 * with their timestamp.  When a messaged is logged, it will also be
 * written to the journal.
 *
 * A saveToTempFile() function will write the log entries as JSON to
 * a temporary file, so they can be added to event logs.
 *
 * The maximum number of entries to keep is specified in the
 * constructor, and after that is hit the oldest entry will be
 * removed when a new one is added.
 */
class Logger
{
  public:
    // timestamp, message
    using LogEntry = std::tuple<std::string, std::string>;

    enum Priority
    {
        error,
        info,
        quiet
    };

    Logger() = delete;
    ~Logger() = default;
    Logger(const Logger&) = default;
    Logger& operator=(const Logger&) = default;
    Logger(Logger&&) = default;
    Logger& operator=(Logger&&) = default;

    /**
     * @brief Constructor
     *
     * @param[in] maxEntries - The maximum number of log entries
     *                         to keep.
     */
    explicit Logger(size_t maxEntries) : _maxEntries(maxEntries)
    {
        assert(maxEntries != 0);
    }

    /**
     * @brief Places an entry in the log and writes it to the journal.
     *
     * @param[in] message - The log message
     *
     * @param[in] priority - The priority for the journal
     */
    void log(const std::string& message, Priority priority = Logger::info)
    {
        if (priority == Logger::error)
        {
            // work-around for lg2 compile error due to use of log name
            // in both lg2 definition and logger.hpp
            lg2::error("{MSG}", "MSG", message);
        }
        else if (priority != Logger::quiet)
        {
            lg2::info("{MSG}", "MSG", message);
        }

        if (_entries.size() == _maxEntries)
        {
            _entries.erase(_entries.begin());
        }

        // Generate a timestamp
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);

        // e.g. Sep 22 19:56:32
        auto timestamp = std::put_time(&tm, "%b %d %H:%M:%S");

        std::ostringstream stream;
        stream << timestamp;
        _entries.emplace_back(stream.str(), message);
    }

    /**
     * @brief Returns the entries in a JSON array
     *
     * @return JSON
     */
    const nlohmann::json getLogs() const
    {
        return _entries;
    }

    /**
     * @brief Writes the data to a temporary file and returns the path
     *        to it.
     *
     * Uses a temp file because the only use case for this is for sending
     * in to an event log where a temp file makes sense, and frankly it
     * was just easier to encapsulate everything here.
     *
     * @return path - The path to the file.
     */
    std::filesystem::path saveToTempFile()
    {
        using namespace std::literals::string_literals;

        char tmpFile[] = "/tmp/loggertemp.XXXXXX";
        util::FileDescriptor fd{mkstemp(tmpFile)};
        if (fd() == -1)
        {
            throw std::runtime_error{"mkstemp failed!"};
        }

        std::filesystem::path path{tmpFile};

        for (const auto& [time, message] : _entries)
        {
            auto line = std::format("{}: {}\n", time, message);
            auto rc = write(fd(), line.data(), line.size());
            if (rc == -1)
            {
                auto e = errno;
                auto msg = std::format(
                    "Could not write to temp file {} errno {}", tmpFile, e);
                log(msg, Logger::error);
                throw std::runtime_error{msg};
            }
        }

        return std::filesystem::path{tmpFile};
    }

    /**
     * @brief Deletes all log entries
     */
    void clear()
    {
        _entries.clear();
    }

  private:
    /**
     * @brief The maximum number of entries to hold
     */
    const size_t _maxEntries;

    /**
     * @brief The vector of <timestamp, message> entries
     */
    std::vector<LogEntry> _entries;
};

} // namespace phosphor::fan
