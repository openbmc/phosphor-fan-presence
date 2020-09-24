#pragma once

#include <fmt/format.h>
#include <unistd.h>

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

#include <cassert>
#include <ctime>
#include <filesystem>
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
        info
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
            phosphor::logging::log<phosphor::logging::level::ERR>(
                message.c_str());
        }
        else
        {
            phosphor::logging::log<phosphor::logging::level::INFO>(
                message.c_str());
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
     * @brief Writes the JSON to a temporary file and returns the path
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
        int fd = mkstemp(tmpFile);
        if (fd == -1)
        {
            throw std::runtime_error{"mkstemp failed!"};
        }

        std::filesystem::path path{tmpFile};

        nlohmann::json data;
        data["Logs"] = _entries;
        auto jsonString = data.dump();

        auto rc = write(fd, jsonString.c_str(), jsonString.size());
        auto e = errno;
        close(fd);
        if (rc == 0)
        {
            log(fmt::format("Could not write to temp file {} errno {}", tmpFile,
                            e),
                Logger::error);
            throw std::runtime_error{"Could not write to "s + path.string()};
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
