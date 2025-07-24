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
#include "fan.hpp"

#include "sdbusplus.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>

#include <format>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

constexpr auto FAN_SENSOR_PATH = "/xyz/openbmc_project/sensors/fan_tach/";
constexpr auto FAN_TARGET_PROPERTY = "Target";

Fan::Fan(const json& jsonObj) :
    ConfigBase(jsonObj), _bus(util::SDBusPlus::getBus())
{
    setInterface(jsonObj);
    setSensors(jsonObj);
    setZone(jsonObj);
}

void Fan::setInterface(const json& jsonObj)
{
    if (!jsonObj.contains("target_interface"))
    {
        lg2::error("Missing required fan sensor target interface", "JSON",
                   jsonObj.dump());
        throw std::runtime_error(
            "Missing required fan sensor target interface");
    }
    _interface = jsonObj["target_interface"].get<std::string>();
}

void Fan::setSensors(const json& jsonObj)
{
    if (!jsonObj.contains("sensors"))
    {
        lg2::error("Missing required fan sensors list", "JSON", jsonObj.dump());
        throw std::runtime_error("Missing required fan sensors list");
    }
    std::string path;
    for (const auto& sensor : jsonObj["sensors"])
    {
        if (!jsonObj.contains("target_path"))
        {
            // If target_path is not set in configuration,
            // it is default to /xyz/openbmc_project/sensors/fan_tach/
            path = FAN_SENSOR_PATH + sensor.get<std::string>();
        }
        else
        {
            path = jsonObj["target_path"].get<std::string>() +
                   sensor.get<std::string>();
        }

        std::string service;
        int attempts = 0;
        constexpr int maxAttempts = 5;
        while (true)
        {
            try
            {
                service = util::SDBusPlus::getService(_bus, path, _interface);
                break;
            }
            catch (const std::exception&)
            {
                lg2::warning("No service for {PATH} {INTERFACE}", "PATH", path,
                             "INTERFACE", _interface);
                attempts++;
                if (attempts == maxAttempts)
                {
                    lg2::error("Giving up");
                    throw;
                }
                lg2::info("Retrying");
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }
        _sensors[path] = service;
    }
    // All sensors associated with this fan are set to the same target,
    // so only need to read target property from one of them
    if (!path.empty())
    {
        _target = util::SDBusPlus::getProperty<uint64_t>(
            _bus, _sensors.at(path), path, _interface, FAN_TARGET_PROPERTY);
    }
}

void Fan::setZone(const json& jsonObj)
{
    if (!jsonObj.contains("zone"))
    {
        lg2::error("Missing required fan zone", "JSON", jsonObj.dump());
        throw std::runtime_error("Missing required fan zone");
    }
    _zone = jsonObj["zone"].get<std::string>();
}

void Fan::setTarget(uint64_t target)
{
    if ((_target == target) || !_lockedTargets.empty())
    {
        return;
    }

    for (const auto& sensor : _sensors)
    {
        auto value = target;
        try
        {
            util::SDBusPlus::setProperty<uint64_t>(
                _bus, sensor.second, sensor.first, _interface,
                FAN_TARGET_PROPERTY, std::move(value));
        }
        catch (const sdbusplus::exception_t&)
        {
            throw util::DBusPropertyError{
                std::format("Failed to set target for fan {}", _name).c_str(),
                sensor.second, sensor.first, _interface, FAN_TARGET_PROPERTY};
        }
    }
    _target = target;
}

void Fan::lockTarget(uint64_t target)
{
    // if multiple locks, take highest, else allow only the
    // first lock to lower the target
    if (target >= _target || _lockedTargets.empty())
    {
        // setTarget wont work if any locked targets exist
        decltype(_lockedTargets) temp;
        _lockedTargets.swap(temp);

        setTarget(target);
        _lockedTargets.swap(temp);
    }

    _lockedTargets.push_back(target);
}

void Fan::unlockTarget(uint64_t target)
{
    // find and remove the requested lock
    auto itr(std::find_if(
        _lockedTargets.begin(), _lockedTargets.end(),
        [target](auto lockedTarget) { return target == lockedTarget; }));

    if (_lockedTargets.end() != itr)
    {
        _lockedTargets.erase(itr);

        // if additional locks, re-lock at next-highest target
        if (!_lockedTargets.empty())
        {
            itr =
                std::max_element(_lockedTargets.begin(), _lockedTargets.end());

            // setTarget wont work if any locked targets exist
            decltype(_lockedTargets) temp;
            _lockedTargets.swap(temp);
            setTarget(*itr);
            _lockedTargets.swap(temp);
        }
    }
}

} // namespace phosphor::fan::control::json
