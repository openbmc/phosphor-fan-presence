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
#include "tach_sensor.hpp"

#include "fan.hpp"
#include "sdbusplus.hpp"
#include "utility.hpp"

#include <fmt/format.h>

#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>

#include <filesystem>
#include <functional>
#include <optional>
#include <utility>

namespace phosphor
{
namespace fan
{
namespace monitor
{

constexpr auto FAN_TARGET_PROPERTY = "Target";
constexpr auto FAN_VALUE_PROPERTY = "Value";
constexpr auto MAX_PREV_TACHS = 8;
constexpr auto MAX_PREV_TARGETS = 8;

namespace fs = std::filesystem;
using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

/**
 * @brief Helper function to read a property
 *
 * @param[in] interface - the interface the property is on
 * @param[in] propertName - the name of the property
 * @param[in] path - the dbus path
 * @param[in] bus - the dbus object
 * @param[out] value - filled in with the property value
 */
template <typename T>
static void
    readProperty(const std::string& interface, const std::string& propertyName,
                 const std::string& path, sdbusplus::bus_t& bus, T& value)
{
    try
    {
        value =
            util::SDBusPlus::getProperty<T>(bus, path, interface, propertyName);
    }
    catch (const std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
    }
}

TachSensor::TachSensor([[maybe_unused]] Mode mode, sdbusplus::bus_t& bus,
                       Fan& fan, const std::string& id, bool hasTarget,
                       size_t funcDelay, const std::string& interface,
                       const std::string& path, double factor, int64_t offset,
                       size_t method, size_t threshold, bool ignoreAboveMax,
                       size_t timeout, const std::optional<size_t>& errorDelay,
                       size_t countInterval, const sdeventplus::Event& event) :
    _bus(bus),
    _fan(fan), _name(FAN_SENSOR_PATH + id),
    _invName(fs::path(fan.getName()) / id), _hasTarget(hasTarget),
    _funcDelay(funcDelay), _interface(interface), _path(path), _factor(factor),
    _offset(offset), _method(method), _threshold(threshold),
    _ignoreAboveMax(ignoreAboveMax), _timeout(timeout),
    _timerMode(TimerMode::func),
    _timer(event, std::bind(&Fan::updateState, &fan, std::ref(*this))),
    _errorDelay(errorDelay), _countInterval(countInterval)
{
    _prevTachs.resize(MAX_PREV_TACHS);

    if (_hasTarget)
    {
        _prevTargets.resize(MAX_PREV_TARGETS);
    }

    updateInventory(_functional);

    // Load in current Target and Input values when entering monitor mode
#ifndef MONITOR_USE_JSON
    if (mode != Mode::init)
    {
#endif
        try
        {
            updateTachAndTarget();
        }
        catch (const std::exception& e)
        {
            // Until the parent Fan's monitor-ready timer expires, the
            // object can be functional with a missing D-bus sensor.
        }

        auto match = getMatchString(std::nullopt, util::FAN_SENSOR_VALUE_INTF);

        tachSignal = std::make_unique<sdbusplus::bus::match_t>(
            _bus, match.c_str(),
            [this](auto& msg) { this->handleTachChange(msg); });

        if (_hasTarget)
        {
            if (_path.empty())
            {
                match = getMatchString(std::nullopt, _interface);
            }
            else
            {
                match = getMatchString(_path, _interface);
            }
            targetSignal = std::make_unique<sdbusplus::bus::match_t>(
                _bus, match.c_str(),
                [this](auto& msg) { this->handleTargetChange(msg); });
        }

        if (_errorDelay)
        {
            _errorTimer = std::make_unique<
                sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>>(
                event, std::bind(&Fan::sensorErrorTimerExpired, &fan,
                                 std::ref(*this)));
        }

        if (_method == MethodMode::count)
        {
            _countTimer = std::make_unique<
                sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>>(
                event,
                std::bind(&Fan::countTimerExpired, &fan, std::ref(*this)));
        }
#ifndef MONITOR_USE_JSON
    }
#endif
}

void TachSensor::updateTachAndTarget()
{
    _tachInput = util::SDBusPlus::getProperty<decltype(_tachInput)>(
        _bus, _name, util::FAN_SENSOR_VALUE_INTF, FAN_VALUE_PROPERTY);

    if (_hasTarget)
    {
        if (_path.empty())
        {
            // Target path is optional
            readProperty(_interface, FAN_TARGET_PROPERTY, _name, _bus,
                         _tachTarget);
        }
        else
        {
            readProperty(_interface, FAN_TARGET_PROPERTY, _path, _bus,
                         _tachTarget);
        }

        // record previous target value
        if (_prevTargets.front() != _tachTarget)
        {
            _prevTargets.push_front(_tachTarget);

            _prevTargets.pop_back();
        }
    }

    // record previous tach value
    _prevTachs.push_front(_tachInput);

    _prevTachs.pop_back();
}

std::string TachSensor::getMatchString(const std::optional<std::string> path,
                                       const std::string& interface)
{
    if (path)
    {
        return sdbusplus::bus::match::rules::propertiesChanged(path.value(),
                                                               interface);
    }
    return sdbusplus::bus::match::rules::propertiesChanged(_name, interface);
}

uint64_t TachSensor::getTarget() const
{
    if (!_hasTarget)
    {
        return _fan.findTargetSpeed();
    }
    return _tachTarget;
}

std::pair<uint64_t, std::optional<uint64_t>>
    TachSensor::getRange(const size_t deviation) const
{
    // Determine min/max range applying the deviation
    uint64_t min = getTarget() * (100 - deviation) / 100;
    std::optional<uint64_t> max = getTarget() * (100 + deviation) / 100;

    // Adjust the min/max range by applying the factor & offset
    min = min * _factor + _offset;
    max = max.value() * _factor + _offset;

    if (_ignoreAboveMax)
    {
        max = std::nullopt;
    }

    return std::make_pair(min, max);
}

void TachSensor::processState()
{
    // This function runs from inside trust::Manager::checkTrust(), which,
    // for sensors using the count method, runs right before process()
    // is called anyway inside Fan::countTimerExpired() so don't call
    // it now if using that method.
    if (_method == MethodMode::timebased)
    {
        _fan.process(*this);
    }
}

void TachSensor::resetMethod()
{
    switch (_method)
    {
        case MethodMode::timebased:
            if (timerRunning())
            {
                stopTimer();
            }
            break;
        case MethodMode::count:
            if (_functional)
            {
                _counter = 0;
            }
            else
            {
                _counter = _threshold;
            }
            break;
    }
}

void TachSensor::setFunctional(bool functional, bool skipErrorTimer)
{
    _functional = functional;
    updateInventory(_functional);

    if (!_errorTimer)
    {
        return;
    }

    if (!_functional)
    {
        if (_fan.present() && !skipErrorTimer)
        {
            _errorTimer->restartOnce(std::chrono::seconds(*_errorDelay));
        }
    }
    else if (_errorTimer->isEnabled())
    {
        _errorTimer->setEnabled(false);
    }
}

void TachSensor::handleTargetChange(sdbusplus::message_t& msg)
{
    readPropertyFromMessage(msg, _interface, FAN_TARGET_PROPERTY, _tachTarget);

    // Check all tach sensors on the fan against the target
    _fan.tachChanged();

    // record previous target value
    if (_prevTargets.front() != _tachTarget)
    {
        _prevTargets.push_front(_tachTarget);

        _prevTargets.pop_back();
    }
}

void TachSensor::handleTachChange(sdbusplus::message_t& msg)
{
    readPropertyFromMessage(msg, util::FAN_SENSOR_VALUE_INTF,
                            FAN_VALUE_PROPERTY, _tachInput);

    // Check just this sensor against the target
    _fan.tachChanged(*this);

    // record previous tach value
    _prevTachs.push_front(_tachInput);

    _prevTachs.pop_back();
}

void TachSensor::startTimer(TimerMode mode)
{
    using namespace std::chrono;

    if (!timerRunning() || mode != _timerMode)
    {
        log<level::DEBUG>(
            fmt::format("Start timer({}) on tach sensor {}. [delay = {}s]",
                        static_cast<int>(mode), _name,
                        duration_cast<seconds>(getDelay(mode)).count())
                .c_str());
        _timer.restartOnce(getDelay(mode));
        _timerMode = mode;
    }
}

std::chrono::microseconds TachSensor::getDelay(TimerMode mode)
{
    using namespace std::chrono;

    switch (mode)
    {
        case TimerMode::nonfunc:
            return duration_cast<microseconds>(seconds(_timeout));
        case TimerMode::func:
            return duration_cast<microseconds>(seconds(_funcDelay));
        default:
            // Log an internal error for undefined timer mode
            log<level::ERR>("Undefined timer mode",
                            entry("TIMER_MODE=%u", mode));
            elog<InternalFailure>();
            return duration_cast<microseconds>(seconds(0));
    }
}

void TachSensor::setCounter(bool count)
{
    if (count)
    {
        if (_counter < _threshold)
        {
            ++_counter;
            log<level::DEBUG>(
                fmt::format(
                    "Incremented error counter on {} to {} (threshold {})",
                    _name, _counter, _threshold)
                    .c_str());
        }
    }
    else
    {
        if (_counter > 0)
        {
            --_counter;
            log<level::DEBUG>(
                fmt::format(
                    "Decremented error counter on {} to {} (threshold {})",
                    _name, _counter, _threshold)
                    .c_str());
        }
    }
}

void TachSensor::startCountTimer()
{
    if (_countTimer)
    {
        log<level::DEBUG>(
            fmt::format("Starting count timer on sensor {}", _name).c_str());
        _countTimer->restart(std::chrono::seconds(_countInterval));
    }
}

void TachSensor::stopCountTimer()
{
    if (_countTimer && _countTimer->isEnabled())
    {
        log<level::DEBUG>(
            fmt::format("Stopping count timer on tach sensor {}.", _name)
                .c_str());
        _countTimer->setEnabled(false);
    }
}

void TachSensor::updateInventory(bool functional)
{
    auto objectMap =
        util::getObjMap<bool>(_invName, util::OPERATIONAL_STATUS_INTF,
                              util::FUNCTIONAL_PROPERTY, functional);

    auto response = util::SDBusPlus::callMethod(
        _bus, util::INVENTORY_SVC, util::INVENTORY_PATH, util::INVENTORY_INTF,
        "Notify", objectMap);

    if (response.is_method_error())
    {
        log<level::ERR>("Error in notify update of tach sensor inventory");
    }
}

} // namespace monitor
} // namespace fan
} // namespace phosphor
