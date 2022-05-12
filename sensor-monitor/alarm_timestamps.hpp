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
#pragma once
#include "config.h"

#include "types.hpp"

#include <fmt/format.h>

#include <cereal/archives/json.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/types/vector.hpp>
#include <phosphor-logging/log.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <filesystem>
#include <fstream>
#include <map>
#include <tuple>

namespace sensor::monitor
{

/**
 * @class AlarmTimestamps
 *
 * This class keeps track of the timestamps at which the shutdown
 * timers are started in case the process or whole BMC restarts
 * while a timer is running.  In the case where the process starts
 * when a timer was previously running and an alarm is still active,
 * a new timer can be started with just the remaining time.
 */
class AlarmTimestamps
{
  public:
    ~AlarmTimestamps() = default;
    AlarmTimestamps(const AlarmTimestamps&) = delete;
    AlarmTimestamps& operator=(const AlarmTimestamps&) = delete;
    AlarmTimestamps(AlarmTimestamps&&) = delete;
    AlarmTimestamps& operator=(AlarmTimestamps&&) = delete;

    /**
     * @brief Constructor
     *
     * Loads any saved timestamps
     */
    AlarmTimestamps()
    {
        load();
    }

    /**
     * @brief Adds an entry to the timestamps map and persists it.
     *
     * @param[in] key - The AlarmKey value
     * @param[in] timestamp - The start timestamp to save
     */
    void add(const AlarmKey& key, uint64_t timestamp)
    {
        // Emplace won't do anything if an entry with that
        // key was already present, so only save if an actual
        // entry was added.
        auto result = timestamps.emplace(key, timestamp);
        if (result.second)
        {
            save();
        }
    }

    /**
     * @brief Erase an entry using the passed in alarm key.
     *
     * @param[in] key - The AlarmKey value
     */
    void erase(const AlarmKey& key)
    {
        size_t removed = timestamps.erase(key);
        if (removed)
        {
            save();
        }
    }

    /**
     * @brief Erase an entry using an iterator.
     */
    void erase(std::map<AlarmKey, uint64_t>::const_iterator& entry)
    {
        timestamps.erase(entry);
        save();
    }

    /**
     * @brief Clear all entries.
     */
    void clear()
    {
        if (!timestamps.empty())
        {
            timestamps.clear();
            save();
        }
    }

    /**
     * @brief Remove any entries for which there is not a running timer
     *        for.  This is used on startup when an alarm could have cleared
     *        during a restart to get rid of the old entries.
     *
     * @param[in] alarms - The current alarms map.
     */
    void prune(
        const std::map<AlarmKey, std::unique_ptr<sdeventplus::utility::Timer<
                                     sdeventplus::ClockId::Monotonic>>>& alarms)
    {
        auto size = timestamps.size();

        auto isTimerStopped = [&alarms](const AlarmKey& key) {
            auto alarm = alarms.find(key);
            if (alarm != alarms.end())
            {
                auto& timer = alarm->second;
                if (timer && timer->isEnabled())
                {
                    return false;
                }
            }
            return true;
        };

        auto it = timestamps.begin();

        while (it != timestamps.end())
        {
            if (isTimerStopped(it->first))
            {
                it = timestamps.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (size != timestamps.size())
        {
            save();
        }
    }

    /**
     * @brief Returns the timestamps map
     */
    const std::map<AlarmKey, uint64_t>& get() const
    {
        return timestamps;
    }

    /**
     * @brief Saves the timestamps map in the filesystem using cereal.
     *
     * Since cereal doesn't understand the AlarmType or ShutdownType
     * enums, they are converted to ints before being written.
     */
    void save()
    {
        std::filesystem::path path =
            std::filesystem::path{SENSOR_MONITOR_PERSIST_ROOT_PATH} /
            timestampsFilename;

        if (!std::filesystem::exists(path.parent_path()))
        {
            std::filesystem::create_directory(path.parent_path());
        }

        std::vector<std::tuple<std::string, int, int, uint64_t>> times;

        for (const auto& [key, time] : timestamps)
        {
            times.emplace_back(std::get<std::string>(key),
                               static_cast<int>(std::get<ShutdownType>(key)),
                               static_cast<int>(std::get<AlarmType>(key)),
                               time);
        }

        std::ofstream stream{path.c_str()};
        cereal::JSONOutputArchive oarchive{stream};

        oarchive(times);
    }

  private:
    static constexpr auto timestampsFilename = "shutdownAlarmStartTimes";

    /**
     * @brief Loads the saved timestamps from the filesystem.
     *
     * As with save(), cereal doesn't understand the ShutdownType or AlarmType
     * enums so they have to have been saved as ints and converted.
     */
    void load()
    {

        std::vector<std::tuple<std::string, int, int, uint64_t>> times;

        std::filesystem::path path =
            std::filesystem::path{SENSOR_MONITOR_PERSIST_ROOT_PATH} /
            timestampsFilename;

        if (!std::filesystem::exists(path))
        {
            return;
        }

        try
        {
            std::ifstream stream{path.c_str()};
            cereal::JSONInputArchive iarchive{stream};
            iarchive(times);

            for (const auto& [path, shutdownType, alarmType, timestamp] : times)
            {
                timestamps.emplace(
                    AlarmKey{path, static_cast<ShutdownType>(shutdownType),
                             static_cast<AlarmType>(alarmType)},
                    timestamp);
            }
        }
        catch (const std::exception& e)
        {
            // Include possible exception when removing file, otherwise ec = 0
            using namespace phosphor::logging;
            std::error_code ec;
            std::filesystem::remove(path, ec);
            log<level::ERR>(
                fmt::format("Unable to restore persisted times ({}, ec: {})",
                            e.what(), ec.value())
                    .c_str());
        }
    }

    /**
     * @brief The map of AlarmKeys and time start times.
     */
    std::map<AlarmKey, uint64_t> timestamps;
};

} // namespace sensor::monitor
