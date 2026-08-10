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

Fan::Fan(const json& jsonObj, ChassisManager& cm) :
    ConfigBase(jsonObj), _cm(cm), _bus(util::SDBusPlus::getBus())
{
    setInterface(jsonObj);
    setChassisPath(jsonObj);
    setZone(jsonObj);

    // Extract the sensor names and optional path prefix here so that
    // setSensors() (called via initSensors()) has no dependency on the
    // original JSON object.
    if (!jsonObj.contains("sensors"))
    {
        lg2::error("Missing required fan sensors list", "JSON", jsonObj.dump());
        throw std::runtime_error("Missing required fan sensors list");
    }
    for (const auto& s : jsonObj["sensors"])
    {
        _sensorNames.push_back(s.get<std::string>());
    }
    if (_sensorNames.empty())
    {
        // An empty list would leave setSensors() with nothing to resolve, so
        // the fan would report neither bound nor pending and would be skipped
        // by every bind path without ever explaining why.
        lg2::error("Empty fan sensors list", "JSON", jsonObj.dump());
        throw std::runtime_error("Empty fan sensors list");
    }
    if (jsonObj.contains("target_path"))
    {
        _targetPath = jsonObj["target_path"].get<std::string>();
    }

    // setSensors() is NOT called here; the caller must call initSensors()
    // after ChassisManager has been fully initialised.
}

void Fan::initSensors(const std::string& hintPath,
                      const std::string& hintService)
{
    // No-op if sensors are already resolved.  Always retry when _sensors is
    // empty so that hotplug callbacks (which construct a fresh Fan object and
    // call initSensors()) can re-attempt the ObjectMapper lookup after the
    // sensor service has appeared.
    if (!_sensors.empty())
    {
        return;
    }
    // Reset any stale pending path so setSensors() starts from scratch.
    _pendingSensorPath.clear();
    setSensors(hintPath, hintService);
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

void Fan::setChassisPath(const json& jsonObj)
{
    if (jsonObj.contains("chassis_path"))
    {
        _chassisPath = jsonObj["chassis_path"].get<std::string>();
        _checkChassisAvailability =
            jsonObj.value("check_chassis_availability", false);
    }
    // If absent, _chassisPath remains empty - not a multi-chassis system,
    // so no chassis gating required for this fan.
}

std::string Fan::sensorPath(const std::string& sensorName) const
{
    // If target_path is not set in configuration, it defaults to
    // /xyz/openbmc_project/sensors/fan_tach/
    return (_targetPath.empty() ? FAN_SENSOR_PATH : _targetPath) + sensorName;
}

std::vector<std::string> Fan::getSensorPaths() const
{
    std::vector<std::string> paths;
    paths.reserve(_sensorNames.size());
    for (const auto& sensorName : _sensorNames)
    {
        paths.push_back(sensorPath(sensorName));
    }
    return paths;
}

void Fan::setSensors(const std::string& hintPath,
                     const std::string& hintService)
{
    // If this fan is associated with a chassis, check whether the chassis is
    // ready before attempting any D-Bus sensor lookups.
    // For systems with no "chassis_path" in JSON, _chassisPath is
    // empty and isReady() returns true immediately.
    if (!_cm.isReady(_chassisPath))
    {
        lg2::debug(
            "Fan {NAME}: chassis {PATH} not ready, deferring sensor lookup",
            "NAME", _name, "PATH", _chassisPath);
        return;
    }

    std::string path;
    for (const auto& sensorName : _sensorNames)
    {
        path = sensorPath(sensorName);

        std::string service;
        try
        {
            service = util::SDBusPlus::getService(_bus, path, _interface);
        }
        catch (const std::exception&)
        {
            // The ObjectMapper does not know about this path.  When the
            // caller already learned the owning service from an
            // InterfacesAdded sender, trust that over the mapper: the mapper
            // processes the same signal independently and routinely has not
            // caught up yet at this point.  Without this, binding off the
            // signal loses the race and the fan is never bound, because the
            // signal has already fired and will not be replayed.
            if (!hintService.empty() && path == hintPath)
            {
                lg2::debug(
                    "Fan {NAME}: {PATH} not on ObjectMapper yet, binding with "
                    "service {SERVICE} from the InterfacesAdded sender",
                    "NAME", _name, "PATH", path, "SERVICE", hintService);
                service = hintService;
            }
            else
            {
                lg2::debug(
                    "Fan {NAME}: sensor {PATH} has no service yet, deferring bind",
                    "NAME", _name, "PATH", path);
                _sensors.clear();
                _pendingSensorPath = path;
                return;
            }
        }
        _sensors[path] = service;
    }
    // All sensors associated with this fan are set to the same target,
    // so only need to read target property from one of them.
    // Keep this inside a try so a mid-flight service drop (between
    // getService() above and getProperty() here) leaves the fan pending
    // rather than throwing out of an sdbusplus match callback.
    if (!path.empty())
    {
        try
        {
            _target = util::SDBusPlus::getProperty<uint64_t>(
                _bus, _sensors.at(path), path, _interface, FAN_TARGET_PROPERTY);
        }
        catch (const std::exception&)
        {
            lg2::debug("Fan {NAME}: sensor {PATH} lost its service between "
                       "lookup and target read, deferring bind",
                       "NAME", _name, "PATH", path);
            _sensors.clear();
            _pendingSensorPath = path;
        }
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
        // setTarget won't work if any locked targets exist
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

            // setTarget won't work if any locked targets exist
            decltype(_lockedTargets) temp;
            _lockedTargets.swap(temp);
            setTarget(*itr);
            _lockedTargets.swap(temp);
        }
    }
}

} // namespace phosphor::fan::control::json
