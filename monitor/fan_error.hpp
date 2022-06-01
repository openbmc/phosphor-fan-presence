#pragma once

#include "utility.hpp"

#include <nlohmann/json.hpp>
#include <xyz/openbmc_project/Logging/Entry/server.hpp>

#include <filesystem>
#include <string>
#include <tuple>

namespace phosphor::fan::monitor
{

/**
 * @class FFDCFile
 *
 * This class holds a file that is used for event log FFDC
 * which needs a file descriptor as input.  The file is
 * deleted upon destruction.
 */
class FFDCFile
{
  public:
    FFDCFile() = delete;
    FFDCFile(const FFDCFile&) = delete;
    FFDCFile& operator=(const FFDCFile&) = delete;
    FFDCFile(FFDCFile&&) = delete;
    FFDCFile& operator=(FFDCFile&&) = delete;

    /**
     * @brief Constructor
     *
     * Opens the file and saves the descriptor
     *
     * @param[in] name - The filename
     */
    explicit FFDCFile(const std::filesystem::path& name);

    /**
     * @brief Destructor - Deletes the file
     */
    ~FFDCFile()
    {
        std::filesystem::remove(_name);
    }

    /**
     * @brief Returns the file descriptor
     *
     * @return int - The descriptor
     */
    int fd()
    {
        return _fd();
    }

  private:
    /**
     * @brief The file descriptor holder
     */
    util::FileDescriptor _fd;

    /**
     * @brief The filename
     */
    const std::filesystem::path _name;
};

/**
 * @class FanError
 *
 * This class represents a fan error.  It has a commit() interface
 * that will create the event log with certain FFDC.
 */
class FanError
{
  public:
    FanError() = delete;
    ~FanError() = default;
    FanError(const FanError&) = delete;
    FanError& operator=(const FanError&) = delete;
    FanError(FanError&&) = delete;
    FanError& operator=(FanError&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] error - The error name, like
     *                    xyz.openbmc_project.Fan.Error.Fault
     * @param[in] fan - The failing fan's inventory path
     * @param[in] sensor - The failing sensor's inventory path.  Can be empty
     *                     if the error is for the FRU and not the sensor.
     * @param[in] severity - The severity of the error
     */
    FanError(const std::string& error, const std::string& fan,
             const std::string& sensor,
             sdbusplus::xyz::openbmc_project::Logging::server::Entry::Level
                 severity) :
        _errorName(error),
        _fanName(fan), _sensorName(sensor),
        _severity(
            sdbusplus::xyz::openbmc_project::Logging::server::convertForMessage(
                severity))
    {}

    /**
     * @brief Constructor
     *
     * This version doesn't take a fan or sensor name.
     *
     * @param[in] error - The error name, like
     *                    xyz.openbmc_project.Fan.Error.Fault
     * @param[in] severity - The severity of the error
     */
    FanError(const std::string& error,
             sdbusplus::xyz::openbmc_project::Logging::server::Entry::Level
                 severity) :
        _errorName(error),
        _severity(
            sdbusplus::xyz::openbmc_project::Logging::server::convertForMessage(
                severity))
    {}

    /**
     * @brief Commits the error by calling the D-Bus method to create
     *        the event log.
     *
     * The FFDC is passed in here so that if an error is committed
     * more than once it can have up to date FFDC.
     *
     * @param[in] jsonFFDC - Free form JSON data that should be sent in as
     *                       FFDC.
     * @param[in] isPowerOffError - If this is committed at the time of the
     *                              power off.
     */
    void commit(const nlohmann::json& jsonFFDC, bool isPowerOffError = false);

  private:
    /**
     * @brief returns a JSON structure containing the previous N journal
     * entries.
     *
     * @param[in] numLines - Number of lines of journal to retrieve
     */
    nlohmann::json getJournalEntries(int numLines) const;

    /**
     * Gets the realtime (wallclock) timestamp for the current journal entry.
     *
     * @param journal current journal entry
     * @return timestamp as a date/time string
     */
    std::string getTimeStamp(sd_journal* journal) const;

    /**
     * Gets the value of the specified field for the current journal entry.
     *
     * Returns an empty string if the current journal entry does not have the
     * specified field.
     *
     * @param journal current journal entry
     * @param field journal field name
     * @return field value
     */
    std::string getFieldValue(sd_journal* journal,
                              const std::string& field) const;

    /**
     * @brief Returns an FFDCFile holding the Logger contents
     *
     * @return std::unique_ptr<FFDCFile> - The file object
     */
    std::unique_ptr<FFDCFile> makeLogFFDCFile();

    /**
     * @brief Returns an FFDCFile holding the contents of the JSON FFDC
     *
     * @param[in] ffdcData - The JSON data to write to a file
     *
     * @return std::unique_ptr<FFDCFile> - The file object
     */
    std::unique_ptr<FFDCFile> makeJsonFFDCFile(const nlohmann::json& ffdcData);

    /**
     * @brief Create and returns the AdditionalData property to use for the
     *        event log.
     *
     * @param[in] isPowerOffError - If this is committed at the time of the
     *                              power off.
     * @return map<string, string> - The AdditionalData contents
     */
    std::map<std::string, std::string> getAdditionalData(bool isPowerOffError);

    /**
     * @brief The error name (The event log's 'Message' property)
     */
    const std::string _errorName;

    /**
     * @brief The inventory name of the failing fan
     */
    const std::string _fanName;

    /**
     * @brief The inventory name of the failing sensor, if there is one.
     */
    const std::string _sensorName;

    /**
     * @brief The severity of the event log.  This is the string
     *        representation of the Entry::Level property.
     */
    const std::string _severity;
};

} // namespace phosphor::fan::monitor
