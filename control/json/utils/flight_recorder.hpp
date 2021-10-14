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
#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdint>
#include <deque>
#include <string>
#include <tuple>
#include <unordered_map>

namespace phosphor::fan::control::json
{
using json = nlohmann::json;

/**
 * @class FlightRecorder
 *
 * This class stores messages and their timestamps based on an ID.
 * When an ID accumulates so many messages, the oldest one will
 * be removed when a new one is added.
 *
 * The dump() function interleaves the messages for all IDs together
 * based on timestamp and then writes them all to /tmp/fan_control.txt.
 *
 * For example:
 * Oct 01 04:37:19.122771:           main: Startup
 * Oct 01 04:37:19.123923: mapped_floor-1: Setting new floor to 4755
 */
class FlightRecorder
{
  public:
    ~FlightRecorder() = default;
    FlightRecorder(const FlightRecorder&) = delete;
    FlightRecorder& operator=(const FlightRecorder&) = delete;
    FlightRecorder(FlightRecorder&&) = delete;
    FlightRecorder& operator=(FlightRecorder&&) = delete;

    /**
     * @brief Returns a reference to the static instance.
     */
    static FlightRecorder& instance();

    /**
     * @brief Logs an entry to the recorder.
     *
     * @param[in] id - The ID of the message owner
     * @param[in] message - The message to log
     */
    void log(const std::string& id, const std::string& message);

    /**
     * @brief Writes the flight recorder contents to JSON.
     *
     * Sorts all messages by timestamp when doing so.
     *
     * @param[out] data - Filled in with the flight recorder data
     */
    void dump(json& data);

  private:
    FlightRecorder() = default;

    // tuple<timestamp, message>
    using Entry = std::tuple<uint64_t, std::string>;

    /* The messages */
    std::unordered_map<std::string, std::deque<Entry>> _entries;
};

} // namespace phosphor::fan::control::json
