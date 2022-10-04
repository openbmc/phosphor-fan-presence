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
#include "anyof.hpp"

#include "fan.hpp"
#include "get_power_state.hpp"
#include "psensor.hpp"

#include <phosphor-logging/log.hpp>

#include <algorithm>

namespace phosphor
{
namespace fan
{
namespace presence
{

using namespace std::chrono_literals;
static const auto powerOnDelayTime = 5s;

AnyOf::AnyOf(const Fan& fan,
             const std::vector<std::reference_wrapper<PresenceSensor>>& s,
             std::unique_ptr<EEPROMDevice> e) :
    RedundancyPolicy(fan, std::move(e)),
    state(), _powerState(getPowerStateObject()),
    _powerOnDelayTimer(sdeventplus::Event::get_default(),
                       std::bind(&AnyOf::delayedAfterPowerOn, this)),
    _powerOn(false)
{
    for (auto& sensor : s)
    {
        state.emplace_back(sensor, false, false);
    }

    _powerState->addCallback(
        std::get<1>(fan) + "-anyOf",
        std::bind(&AnyOf::powerStateChanged, this, std::placeholders::_1));

    // If power is already on, give the fans some time to spin up
    // before considering power to actually be on.
    if (_powerState->isPowerOn())
    {
        _powerOnDelayTimer.restartOnce(powerOnDelayTime);
    }
}

void AnyOf::stateChanged(bool present, PresenceSensor& sensor)
{
    // Find the sensor that changed state.
    auto sit =
        std::find_if(state.begin(), state.end(), [&sensor](const auto& s) {
            return std::get<sensorPos>(s).get() == sensor;
        });
    if (sit != state.end())
    {
        auto origState =
            std::any_of(state.begin(), state.end(),
                        [](const auto& s) { return std::get<presentPos>(s); });

        // Update our cache of the sensors state and re-evaluate.
        std::get<presentPos>(*sit) = present;
        auto newState =
            std::any_of(state.begin(), state.end(),
                        [](const auto& s) { return std::get<presentPos>(s); });
        setPresence(fan, newState);

        if (eepromDevice && (newState != origState))
        {
            if (newState)
            {
                eepromDevice->bind();
            }
            else
            {
                eepromDevice->unbind();
            }
        }

        // At least 1 sensor said a fan was present, check if any disagree.
        if (newState)
        {
            if (!origState)
            {
                // Fan plug detected, re-enable conflict logging
                std::for_each(state.begin(), state.end(), [](auto& s) {
                    std::get<conflictPos>(s) = false;
                });
            }

            checkSensorConflicts();
        }
    }
}

void AnyOf::monitor()
{
    // Start all sensors in the anyof redundancy set.

    for (auto& s : state)
    {
        auto& sensor = std::get<sensorPos>(s);
        std::get<presentPos>(s) = sensor.get().start();
    }

    auto present = std::any_of(state.begin(), state.end(), [](const auto& s) {
        return std::get<presentPos>(s);
    });
    setPresence(fan, present);

    // At least one of the contained methods indicated present,
    // so check that they all agree.
    if (present)
    {
        checkSensorConflicts();
    }
}

void AnyOf::checkSensorConflicts()
{
    if (!isPowerOn())
    {
        return;
    }

    // If at least one, but not all, sensors indicate present, then
    // tell the not present ones to log a conflict if not already done.
    if (std::any_of(
            state.begin(), state.end(),
            [this](const auto& s) { return std::get<presentPos>(s); }) &&
        !std::all_of(state.begin(), state.end(),
                     [this](const auto& s) { return std::get<presentPos>(s); }))
    {
        for (auto& [sensor, present, conflict] : state)
        {
            if (!present && !conflict)
            {
                sensor.get().logConflict(invNamespace + std::get<1>(fan));
                conflict = true;
            }
        }
    }
}

void AnyOf::powerStateChanged(bool powerOn)
{
    if (powerOn)
    {
        // Clear the conflict state from last time
        std::for_each(state.begin(), state.end(), [](auto& state) {
            std::get<conflictPos>(state) = false;
        });

        // Wait to give the fans time to start spinning before
        // considering power actually on.
        _powerOnDelayTimer.restartOnce(powerOnDelayTime);
    }
    else
    {
        _powerOn = false;

        if (_powerOnDelayTimer.isEnabled())
        {
            _powerOnDelayTimer.setEnabled(false);
        }
    }
}

void AnyOf::delayedAfterPowerOn()
{
    _powerOn = true;
    checkSensorConflicts();
}

} // namespace presence
} // namespace fan
} // namespace phosphor
