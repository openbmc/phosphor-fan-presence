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
#include "flight_recorder.hpp"

#include <fmt/format.h>

#include <phosphor-logging/log.hpp>

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>

constexpr auto maxEntriesPerID = 40;

namespace phosphor::fan::control::json
{
using json = nlohmann::json;

FlightRecorder& FlightRecorder::instance()
{
    static FlightRecorder fr;
    return fr;
}

void FlightRecorder::log(const std::string& id, const std::string& message)
{
    uint64_t ts = std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();

    auto& entry = _entries[id];
    entry.emplace_back(ts, message);
    if (entry.size() > maxEntriesPerID)
    {
        entry.pop_front();
    }
}

void FlightRecorder::dump(json& data)
{
    using namespace std::chrono;
    using Timepoint = time_point<system_clock, microseconds>;

    size_t idSize = 0;
    std::vector<std::tuple<Timepoint, std::string, std::string>> output;

    for (const auto& [id, messages] : _entries)
    {
        for (const auto& [ts, msg] : messages)
        {
            idSize = std::max(idSize, id.size());
            Timepoint tp{microseconds{ts}};
            output.emplace_back(tp, id, msg);
        }
    }

    std::sort(output.begin(), output.end(),
              [](const auto& left, const auto& right) {
                  return std::get<Timepoint>(left) < std::get<Timepoint>(right);
              });

    auto formatTime = [](const Timepoint& tp) {
        std::stringstream ss;
        std::time_t tt = system_clock::to_time_t(tp);
        uint64_t us =
            duration_cast<microseconds>(tp.time_since_epoch()).count();

        // e.g. Oct 04 16:43:45.923555
        ss << std::put_time(std::localtime(&tt), "%b %d %H:%M:%S.");
        ss << std::setfill('0') << std::setw(6) << std::to_string(us % 1000000);
        return ss.str();
    };

    auto& fr = data["flight_recorder"];
    std::stringstream ss;

    for (const auto& [ts, id, msg] : output)
    {
        ss << formatTime(ts) << ": " << std::setw(idSize) << id << ": " << msg;
        fr.push_back(ss.str());
        ss.str("");
    }
}

} // namespace phosphor::fan::control::json
