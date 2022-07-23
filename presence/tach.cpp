/**
 * Copyright © 2017 IBM Corporation
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
#include "tach.hpp"

#include "logging.hpp"
#include "rpolicy.hpp"

#include <fmt/format.h>

#include <phosphor-logging/log.hpp>

#include <string>
#include <tuple>
#include <vector>

namespace phosphor
{
namespace fan
{
namespace presence
{

using namespace phosphor::logging;
using namespace std::literals::string_literals;

static const auto tachNamespace = "/xyz/openbmc_project/sensors/fan_tach/"s;
static const auto tachIface = "xyz.openbmc_project.Sensor.Value"s;
static const auto tachProperty = "Value"s;

Tach::Tach(const std::vector<std::string>& sensors) : currentState(false)
{
    // Initialize state.
    for (const auto& s : sensors)
    {
        state.emplace_back(s, nullptr, 0);
    }
}

bool Tach::start()
{
    for (size_t i = 0; i < state.size(); ++i)
    {
        auto& s = state[i];
        auto tachPath = tachNamespace + std::get<std::string>(s);

        // Register for signal callbacks.
        std::get<1>(s) = std::make_unique<sdbusplus::bus::match_t>(
            util::SDBusPlus::getBus(),
            sdbusplus::bus::match::rules::propertiesChanged(tachPath,
                                                            tachIface),
            [this, i](auto& msg) { this->propertiesChanged(i, msg); });

        // Get an initial tach speed.
        try
        {
            std::get<double>(s) = util::SDBusPlus::getProperty<double>(
                tachPath, tachIface, tachProperty);
        }
        catch (const std::exception&)
        {
            // Assume not spinning.

            std::get<double>(s) = 0;
            log<level::INFO>(
                fmt::format("Unable to read fan tach sensor {}", tachPath)
                    .c_str());
        }
    }

    // Set the initial state of the sensor.
    currentState = std::any_of(state.begin(), state.end(), [](const auto& s) {
        return std::get<double>(s) != 0;
    });

    return currentState;
}

void Tach::stop()
{
    for (auto& s : state)
    {
        // De-register signal callbacks.
        std::get<1>(s) = nullptr;
    }
}

bool Tach::present()
{
    // Live query the tach readings.
    std::vector<double> values;
    for (const auto& s : state)
    {
        values.push_back(util::SDBusPlus::getProperty<double>(
            tachNamespace + std::get<std::string>(s), tachIface, tachProperty));
    }

    return std::any_of(values.begin(), values.end(),
                       [](const auto& v) { return v != 0; });
}

void Tach::propertiesChanged(size_t sensor, sdbusplus::message_t& msg)
{
    std::string iface;
    util::Properties<double> properties;
    msg.read(iface, properties);

    propertiesChanged(sensor, properties);
}

void Tach::propertiesChanged(size_t sensor,
                             const util::Properties<double>& props)
{
    // Find the Value property containing the speed.
    auto it = props.find(tachProperty);
    if (it != props.end())
    {
        auto& s = state[sensor];
        std::get<double>(s) = std::get<double>(it->second);

        auto newState =
            std::any_of(state.begin(), state.end(),
                        [](const auto& s) { return std::get<double>(s) != 0; });

        if (currentState != newState)
        {
            getPolicy().stateChanged(newState, *this);
            currentState = newState;
        }
    }
}

void Tach::logConflict(const std::string& fanInventoryPath) const
{
    getLogger().log(fmt::format(
        "Tach sensor presence detect for fan {} said not present but "
        "other methods indicated present",
        fanInventoryPath));

    // Let the code that monitors fan faults create the event
    // logs for stopped rotors.
}

} // namespace presence
} // namespace fan
} // namespace phosphor
