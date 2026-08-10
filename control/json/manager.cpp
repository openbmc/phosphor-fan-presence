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
#include "config.h"

#include "manager.hpp"

#include "action.hpp"
#include "dbus_paths.hpp"
#include "event.hpp"
#include "fan.hpp"
#include "group.hpp"
#include "json_config.hpp"
#include "power_state.hpp"
#include "profile.hpp"
#include "sdbusplus.hpp"
#include "utils/flight_recorder.hpp"
#include "zone.hpp"

#include <systemd/sd-bus.h>

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

std::vector<std::string> Manager::_activeProfiles;
std::map<std::string,
         std::map<std::string, std::pair<bool, std::vector<std::string>>>>
    Manager::_servTree;
std::map<std::string,
         std::map<std::string, std::map<std::string, PropertyVariantType>>>
    Manager::_objects;
std::unordered_map<std::string, PropertyVariantType> Manager::_parameters;
std::unordered_map<std::string, TriggerActions> Manager::_parameterTriggers;

const std::string Manager::dumpFile = "/tmp/fan_control_dump.json";

Manager::Manager(const sdeventplus::Event& event) :
    _bus(util::SDBusPlus::getBus()), _event(event),
    _mgr(util::SDBusPlus::getBus(), CONTROL_OBJPATH), _loadAllowed(true),
    _powerState(std::make_unique<PGoodState>(
        util::SDBusPlus::getBus(),
        std::bind(std::mem_fn(&Manager::powerStateChanged), this,
                  std::placeholders::_1)))
{}

void Manager::sighupHandler(sdeventplus::source::Signal&,
                            const struct signalfd_siginfo*)
{
    FlightRecorder::instance().log("main", "SIGHUP received");
    // Save current set of available and active profiles
    std::map<configKey, std::unique_ptr<Profile>> profiles;
    profiles.swap(_profiles);
    std::vector<std::string> activeProfiles;
    activeProfiles.swap(_activeProfiles);

    try
    {
        _loadAllowed = true;
        load();
    }
    catch (const std::runtime_error& re)
    {
        // Restore saved available and active profiles
        _loadAllowed = false;
        _profiles.swap(profiles);
        _activeProfiles.swap(activeProfiles);
        lg2::error("Error reloading configs, no changes made: {LOAD_ERROR}",
                   "LOAD_ERROR", re);
        FlightRecorder::instance().log(
            "main", std::format("Error reloading configs, no changes made: {}",
                                re.what()));
    }
}

void Manager::dumpDebugData(sdeventplus::source::Signal&,
                            const struct signalfd_siginfo*)
{
    json data;
    FlightRecorder::instance().dump(data);
    dumpCache(data);

    std::for_each(_zones.begin(), _zones.end(), [&data](const auto& zone) {
        data["zones"][zone.second->getName()] = zone.second->dump();
    });

    std::ofstream file{Manager::dumpFile};
    if (!file)
    {
        lg2::error("Could not open file for fan dump");
        return;
    }

    file << std::setw(4) << data;
}

void Manager::dumpCache(json& data)
{
    auto& objects = data["objects"];
    for (const auto& [path, interfaces] : _objects)
    {
        auto& interfaceJSON = objects[path];

        for (const auto& [interface, properties] : interfaces)
        {
            auto& propertyJSON = interfaceJSON[interface];
            for (const auto& [propName, propValue] : properties)
            {
                std::visit(
                    [&obj = propertyJSON[propName]](auto&& val) { obj = val; },
                    propValue);
            }
        }
    }

    auto& parameters = data["parameters"];
    for (const auto& [name, value] : _parameters)
    {
        std::visit([&obj = parameters[name]](auto&& val) { obj = val; }, value);
    }

    std::for_each(_events.begin(), _events.end(), [&data](const auto& event) {
        data["events"][event.second->getName()] = event.second->dump();
    });

    data["services"] = _servTree;
}

void Manager::load()
{
    if (_loadAllowed)
    {
        // Load the available profiles and which are active
        setProfiles();

        // Load the zone configurations
        auto zones = getConfig<Zone>(false, _event, this);

        // Scan fans.json to collect the set of unique chassis paths with
        // their availability-check flag, and build the chassis->zones map.
        // ChassisManager must be initialised before getConfig<Fan>() so that
        // Fan::setSensors() sees real D-Bus present/available state when it
        // calls ChassisManager::isReady().
        {
            // Always construct a fresh ChassisManager so Fan constructors
            // always receive a valid reference.  registerChassis() is only
            // called when the config actually has chassis_path entries.
            _chassisMgr = std::make_unique<ChassisManager>();
            _chassisMgr->init(
                _bus,
                [this](const std::string& chassisPath) {
                    handleChassisStatusChange(chassisPath);
                },
                [this](const std::string& chassisPath,
                       const std::string& sensorPath,
                       const std::string& targetInterface) {
                    handleFanSensorLost(chassisPath, sensorPath,
                                        targetInterface);
                });

            auto confFile = fan::JsonConfig::getConfFile(
                confAppName, Fan::confFileName, false);
            if (!confFile.empty())
            {
                std::map<std::string, bool> chassisPaths;
                _chassisPathToZones.clear();
                for (const auto& entry : fan::JsonConfig::load(confFile))
                {
                    if (entry.contains("chassis_path"))
                    {
                        std::string path =
                            entry["chassis_path"].get<std::string>();
                        bool checkAvail =
                            entry.value("check_chassis_availability", false);
                        auto [it, inserted] = chassisPaths.emplace(path, false);
                        if (checkAvail)
                        {
                            it->second = true;
                        }
                        if (entry.contains("zone"))
                        {
                            _chassisPathToZones[path].insert(
                                entry["zone"].get<std::string>());
                        }
                    }
                }
                for (const auto& [path, checkAvail] : chassisPaths)
                {
                    _chassisMgr->registerChassis(path, checkAvail);
                }
            }
        }

        _multiChassis = !_chassisMgr->empty();
        if (_multiChassis)
        {
            lg2::debug(
                "Fan control running in multi-chassis mode ({NUM} chassis registered)",
                "NUM", _chassisMgr->size());
        }
        else
        {
            lg2::debug(
                "Fan control running in single-chassis mode (no chassis_path in config)");
        }

        // Load the fan configurations and move each fan into its zone,
        // skipping fans whose chassis is not ready (not present, or not
        // available when so configured).
        auto fans = getConfig<Fan>(false, *_chassisMgr);

        // Collect watches to install after _zones is fully populated.
        // Installing them inline would allow callbacks to fire re-entrantly
        // during load() while _zones is still empty, causing a crash.
        // isLossWatch=true  → InterfacesRemoved (sensor disappeared)
        // isLossWatch=false → InterfacesAdded   (sensor not yet present)
        struct PendingWatch
        {
            std::string chassisPath;
            std::string sensorPath;
            std::string interface;
            bool isLossWatch{false};
        };
        std::vector<PendingWatch> pendingWatches;

        for (auto& fan : fans)
        {
            if (!fan.second->hasSensorsOnDbus())
            {
                if (fan.second->getPendingSensorPath().empty())
                {
                    // Chassis was not ready at construction time so the sensor
                    // lookup was never attempted.  Wait for the chassis
                    // Present/Available signal to re-trigger this function.
                    continue;
                }

                // Chassis was ready but the sensor service had not yet
                // appeared on D-Bus.  Defer the watch installation until
                // after _zones is fully populated (see below).
                if (_chassisMgr)
                {
                    pendingWatches.push_back(
                        {fan.second->getChassisPath(),
                         fan.second->getPendingSensorPath(),
                         fan.second->getInterface()});
                }
                continue;
            }
            configKey fanProfile =
                std::make_pair(fan.second->getZone(), fan.first.second);
            auto itZone = std::find_if(
                zones.begin(), zones.end(), [&fanProfile](const auto& zone) {
                    return Manager::inConfig(fanProfile, zone.first);
                });
            if (itZone != zones.end())
            {
                if (itZone->second->getTarget() != fan.second->getTarget() &&
                    fan.second->getTarget() != 0)
                {
                    // Update zone target to current target of the fan in the
                    // zone
                    itZone->second->setTarget(fan.second->getTarget());
                }
                // Collect loss-watch info before the unique_ptr is moved
                if (_chassisMgr)
                {
                    for (const auto& [sp, svc] : fan.second->getSensors())
                    {
                        pendingWatches.push_back(
                            {fan.second->getChassisPath(), sp,
                             fan.second->getInterface(), true});
                    }
                }
                itZone->second->addFan(std::move(fan.second));
            }
        }

        // Save all currently available groups, if any, then clear for reloading
        auto groups = std::move(Event::getAllGroups(false));
        Event::clearAllGroups();

        std::map<configKey, std::unique_ptr<Event>> events;
        try
        {
            // Load any events configured, including all the groups
            events = getConfig<Event>(true, this, zones);
        }
        catch (const std::runtime_error& re)
        {
            // Restore saved set of all available groups for current events
            Event::setAllGroups(std::move(groups));
            throw re;
        }

        // Enable zones
        _zones = std::move(zones);
        std::for_each(_zones.begin(), _zones.end(),
                      [](const auto& entry) { entry.second->enable(); });

        // Clear current timers and signal subscriptions before enabling events
        // To save reloading services and/or objects into cache, do not clear
        // cache
        _timers.clear();
        _signals.clear();

        // Enable events
        _events = std::move(events);
        FlightRecorder::instance().log("main", "Enabling events");
        std::for_each(_events.begin(), _events.end(),
                      [](const auto& entry) { entry.second->enable(); });
        FlightRecorder::instance().log("main", "Done enabling events");

        _loadAllowed = false;

        // Install all deferred watches now that _zones is fully populated and
        // _loadAllowed is false.  Any signal that fires immediately will enter
        // the handler cleanly.
        for (const auto& pw : pendingWatches)
        {
            if (pw.isLossWatch)
            {
                _chassisMgr->watchFanSensorLoss(pw.chassisPath, pw.sensorPath,
                                                pw.interface);
            }
            else
            {
                _chassisMgr->watchFanSensor(pw.chassisPath, pw.sensorPath,
                                            pw.interface);
            }
        }
    }
}

void Manager::handleChassisStatusChange(const std::string& chassisPath)
{
    // For single-chassis systems (no chassis_path in JSON), chassisPath is
    // empty and _chassisPathToZones has no entry for it.  Skip the chassis
    // state checks - isReady("") always returns true - and go straight to
    // the fan bind loop below.
    if (!chassisPath.empty())
    {
        auto mapIt = _chassisPathToZones.find(chassisPath);
        if (mapIt == _chassisPathToZones.end())
        {
            // No zones registered for this path - nothing to do.
            lg2::debug("Manager::handleChassisStatusChange: no zones found for "
                       "chassis {PATH}",
                       "PATH", chassisPath);
            return;
        }

        // Chassis went unavailable - remove its fans from all zones and
        // discard any active loss watches (fans are gone).
        if (!_chassisMgr->isReady(chassisPath))
        {
            lg2::info("Manager::handleChassisStatusChange: chassis {PATH} went "
                      "unavailable, removing its fans from zones",
                      "PATH", chassisPath);
            _chassisMgr->clearFanSensorLossWatches(chassisPath);
            for (const auto& zoneName : mapIt->second)
            {
                for (auto& [key, zone] : _zones)
                {
                    if (zone->getName() == zoneName)
                    {
                        zone->removeFansByChassis(chassisPath);
                    }
                }
            }
            return;
        }
    }

    lg2::info("Manager::handleChassisStatusChange: chassis {PATH} is ready, "
              "adding fans to its zones",
              "PATH", chassisPath);

    // Construct all fans from JSON.  For fans belonging to this chassis,
    // Fan::setSensors() will attempt service lookups now that the chassis is
    // ready.  Fans whose sensor service is still absent will have
    // hasSensorsOnDbus() == false and getPendingSensorPath() non-empty.
    auto fans = getConfig<Fan>(false, *_chassisMgr);

    // Track which zones received at least one new fan so we can push the
    // zone's current target to them after the loop.
    std::set<Zone*> updatedZones;

    // Collect watches to install after the fan loop completes.
    // isLossWatch=true  → InterfacesRemoved watch for a bound fan
    // isLossWatch=false → InterfacesAdded watch for a still-absent sensor
    struct PendingWatch
    {
        std::string chassisPath;
        std::string sensorPath;
        std::string interface;
        bool isLossWatch{false};
    };
    std::vector<PendingWatch> pendingWatches;

    for (auto& fan : fans)
    {
        if (fan.second->getChassisPath() != chassisPath)
        {
            continue;
        }

        if (!fan.second->hasSensorsOnDbus())
        {
            // Sensor service not yet on D-Bus; defer the watch installation
            // until after this function's local state is unwound (see below).
            const auto& sensorPath = fan.second->getPendingSensorPath();
            if (!sensorPath.empty())
            {
                pendingWatches.push_back(
                    {chassisPath, sensorPath, fan.second->getInterface()});
            }
            continue;
        }

        // Find the matching zone in _zones (already running).
        configKey fanProfile =
            std::make_pair(fan.second->getZone(), fan.first.second);
        auto itZone = std::find_if(
            _zones.begin(), _zones.end(), [&fanProfile](const auto& zone) {
                return Manager::inConfig(fanProfile, zone.first);
            });
        if (itZone != _zones.end())
        {
            updatedZones.insert(itZone->second.get());
            // Collect loss-watch info before the unique_ptr is moved
            for (const auto& [sp, svc] : fan.second->getSensors())
            {
                pendingWatches.push_back(
                    {chassisPath, sp, fan.second->getInterface(), true});
            }
            itZone->second->addFan(std::move(fan.second));
        }
    }

    // Clear any stale sensor watches only when all fans for this chassis are
    // fully bound (no pending sensor path remains).  If some fans are still
    // waiting for their service to appear the watches must be preserved.
    // Note: fans that were successfully bound have been moved into zones above,
    // so only check entries where the unique_ptr is still non-null.
    bool anyPending =
        std::any_of(fans.begin(), fans.end(), [&chassisPath](const auto& f) {
            return f.second != nullptr &&
                   f.second->getChassisPath() == chassisPath &&
                   !f.second->getPendingSensorPath().empty();
        });
    if (!anyPending)
    {
        _chassisMgr->clearFanSensorWatches(chassisPath);
    }

    // If host power is already on, push the zone's live target to the
    // newly-added fans so they are immediately driven to the correct speed.
    // If power is off, powerStateChanged() will apply poweronTarget when
    // the host powers on, just as it does for all other zones.
    if (isPowerOn())
    {
        for (auto* zone : updatedZones)
        {
            zone->setTarget(zone->getTarget());
        }
    }

    // Install all deferred watches now that local state is fully processed.
    for (const auto& pw : pendingWatches)
    {
        if (pw.isLossWatch)
        {
            _chassisMgr->watchFanSensorLoss(pw.chassisPath, pw.sensorPath,
                                            pw.interface);
        }
        else
        {
            _chassisMgr->watchFanSensor(pw.chassisPath, pw.sensorPath,
                                        pw.interface);
        }
    }
}

void Manager::handleFanSensorLost(const std::string& chassisPath,
                                  const std::string& sensorPath,
                                  const std::string& targetInterface)
{
    lg2::info("Manager::handleFanSensorLost: sensor {SENSOR} lost "
              "(chassis {PATH}), removing fan and installing recovery watch",
              "SENSOR", sensorPath, "PATH", chassisPath);

    // Remove the fan from its zone
    for (auto& [key, zone] : _zones)
    {
        zone->removeFanBySensorPath(sensorPath);
    }

    // Discard the now-stale loss watch (safe: called after callback returns)
    _chassisMgr->clearFanSensorLossWatches(chassisPath);

    // Install an InterfacesAdded recovery watch so the fan is re-bound
    // when the sensor service returns.
    _chassisMgr->watchFanSensor(chassisPath, sensorPath, targetInterface);
}

void Manager::powerStateChanged(bool powerStateOn)
{
    if (powerStateOn)
    {
        FlightRecorder::instance().log("power", "Power on");
        if (_zones.empty())
        {
            throw std::runtime_error("No configured zones found at poweron");
        }
        std::for_each(_zones.begin(), _zones.end(), [](const auto& entry) {
            entry.second->setTarget(entry.second->getPoweronTarget());
        });

        // Tell events to run their power on triggers
        std::for_each(_events.begin(), _events.end(),
                      [](const auto& entry) { entry.second->powerOn(); });
    }
    else
    {
        FlightRecorder::instance().log("power", "Power off");
        // Tell events to run their power off triggers
        std::for_each(_events.begin(), _events.end(),
                      [](const auto& entry) { entry.second->powerOff(); });
    }
}

const std::vector<std::string>& Manager::getActiveProfiles()
{
    return _activeProfiles;
}

bool Manager::inConfig(const configKey& input, const configKey& comp)
{
    // Config names dont match, do not include in config
    if (input.first != comp.first)
    {
        return false;
    }
    // No profiles specified by input config, can be used in any config
    if (input.second.empty())
    {
        return true;
    }
    else
    {
        // Profiles must have one match in the other's profiles(and they must be
        // an active profile) to be used in the config
        return std::any_of(
            input.second.begin(), input.second.end(),
            [&comp](const auto& lProfile) {
                return std::any_of(
                    comp.second.begin(), comp.second.end(),
                    [&lProfile](const auto& rProfile) {
                        if (lProfile != rProfile)
                        {
                            return false;
                        }
                        auto activeProfs = getActiveProfiles();
                        return std::find(activeProfs.begin(), activeProfs.end(),
                                         lProfile) != activeProfs.end();
                    });
            });
    }
}

bool Manager::hasOwner(const std::string& path, const std::string& intf)
{
    auto itServ = _servTree.find(path);
    if (itServ == _servTree.end())
    {
        // Path not found in cache, therefore owner missing
        return false;
    }
    for (const auto& service : itServ->second)
    {
        auto itIntf = std::find_if(
            service.second.second.begin(), service.second.second.end(),
            [&intf](const auto& interface) { return intf == interface; });
        if (itIntf != std::end(service.second.second))
        {
            // Service found, return owner state
            return service.second.first;
        }
    }
    // Interface not found in cache, therefore owner missing
    return false;
}

void Manager::setOwner(const std::string& serv, bool hasOwner)
{
    // Update owner state on all entries of `serv`
    for (auto& itPath : _servTree)
    {
        auto itServ = itPath.second.find(serv);
        if (itServ != itPath.second.end())
        {
            itServ->second.first = hasOwner;

            // Remove associated interfaces from object cache when service no
            // longer has an owner
            if (!hasOwner && _objects.find(itPath.first) != _objects.end())
            {
                for (auto& intf : itServ->second.second)
                {
                    _objects[itPath.first].erase(intf);
                }
            }
        }
    }
}

void Manager::setOwner(const std::string& path, const std::string& serv,
                       const std::string& intf, bool isOwned)
{
    // Set owner state for specific object given
    auto& ownIntf = _servTree[path][serv];
    ownIntf.first = isOwned;
    auto itIntf = std::find_if(
        ownIntf.second.begin(), ownIntf.second.end(),
        [&intf](const auto& interface) { return intf == interface; });
    if (itIntf == std::end(ownIntf.second))
    {
        ownIntf.second.emplace_back(intf);
    }

    // Update owner state on all entries of the same `serv` & `intf`
    for (auto& itPath : _servTree)
    {
        if (itPath.first == path)
        {
            // Already set/updated owner on this path for `serv` & `intf`
            continue;
        }
        for (auto& itServ : itPath.second)
        {
            if (itServ.first != serv)
            {
                continue;
            }
            auto itIntf = std::find_if(
                itServ.second.second.begin(), itServ.second.second.end(),
                [&intf](const auto& interface) { return intf == interface; });
            if (itIntf != std::end(itServ.second.second))
            {
                itServ.second.first = isOwned;
            }
        }
    }
}

const std::string& Manager::findService(const std::string& path,
                                        const std::string& intf)
{
    static const std::string empty = "";

    auto itServ = _servTree.find(path);
    if (itServ != _servTree.end())
    {
        for (const auto& service : itServ->second)
        {
            auto itIntf = std::find_if(
                service.second.second.begin(), service.second.second.end(),
                [&intf](const auto& interface) { return intf == interface; });
            if (itIntf != std::end(service.second.second))
            {
                // Service found, return service name
                return service.first;
            }
        }
    }

    return empty;
}

void Manager::addServices(const std::string& intf, int32_t depth)
{
    // Get all subtree objects for the given interface
    auto objects = util::SDBusPlus::getSubTreeRaw(util::SDBusPlus::getBus(),
                                                  "/", intf, depth);
    // Add what's returned to the cache of path->services
    for (auto& itPath : objects)
    {
        auto pathIter = _servTree.find(itPath.first);
        if (pathIter != _servTree.end())
        {
            // Path found in cache
            for (auto& itServ : itPath.second)
            {
                auto servIter = pathIter->second.find(itServ.first);
                if (servIter != pathIter->second.end())
                {
                    if (std::find(servIter->second.second.begin(),
                                  servIter->second.second.end(), intf) ==
                        servIter->second.second.end())
                    {
                        // Add interface to cache
                        servIter->second.second.emplace_back(intf);
                    }
                }
                else
                {
                    // Service not found in cache
                    auto intfs = {intf};
                    pathIter->second[itServ.first] =
                        std::make_pair(true, intfs);
                }
            }
        }
        else
        {
            // Path not found in cache
            auto intfs = {intf};
            for (const auto& [servName, servIntfs] : itPath.second)
            {
                _servTree[itPath.first][servName] = std::make_pair(true, intfs);
            }
        }
    }
}

const std::string& Manager::getService(const std::string& path,
                                       const std::string& intf)
{
    // Retrieve service from cache
    const auto& serviceName = findService(path, intf);
    if (serviceName.empty())
    {
        addServices(intf, 0);
        return findService(path, intf);
    }

    return serviceName;
}

std::vector<std::string> Manager::findPaths(const std::string& serv,
                                            const std::string& intf)
{
    std::vector<std::string> paths;

    for (const auto& path : _servTree)
    {
        auto itServ = path.second.find(serv);
        if (itServ != path.second.end())
        {
            if (std::find(itServ->second.second.begin(),
                          itServ->second.second.end(), intf) !=
                itServ->second.second.end())
            {
                if (std::find(paths.begin(), paths.end(), path.first) ==
                    paths.end())
                {
                    paths.push_back(path.first);
                }
            }
        }
    }

    return paths;
}

std::vector<std::string> Manager::getPaths(const std::string& serv,
                                           const std::string& intf)
{
    auto paths = findPaths(serv, intf);
    if (paths.empty())
    {
        addServices(intf, 0);
        return findPaths(serv, intf);
    }

    return paths;
}

void Manager::insertFilteredObjects(ManagedObjects& ref)
{
    // Filter out objects that aren't part of a group
    const auto& allGroupMembers = Group::getAllMembers();
    auto it = ref.begin();

    while (it != ref.end())
    {
        if (allGroupMembers.find(it->first) == allGroupMembers.end())
        {
            it = ref.erase(it);
        }
        else
        {
            it++;
        }
    }

    for (auto& [path, pathMap] : ref)
    {
        for (auto& [intf, intfMap] : pathMap)
        {
            // for each property on this path+interface
            for (auto& [prop, value] : intfMap)
            {
                setProperty(path, intf, prop, value);
            }
        }
    }
}

void Manager::addObjects(const std::string& path, const std::string& intf,
                         const std::string& prop,
                         const std::string& serviceName)
{
    auto service = serviceName;
    if (service.empty())
    {
        service = getService(path, intf);
        if (service.empty())
        {
            // Log service not found for object
            lg2::debug(
                "Unable to get service name for path {PATH}, interface {INTERFACE}",
                "PATH", path, "INTERFACE", intf);
            return;
        }
    }
    else
    {
        // The service is known, so the service cache can be
        // populated even if the path itself isn't present.
        const auto& s = findService(path, intf);
        if (s.empty())
        {
            addServices(intf, 0);
        }
    }

    auto objMgrPaths = getPaths(service, "org.freedesktop.DBus.ObjectManager");

    bool useManagedObj = false;

    if (!objMgrPaths.empty())
    {
        for (const auto& objMgrPath : objMgrPaths)
        {
            // Get all managed objects of service
            auto objects =
                util::SDBusPlus::getManagedObjects<PropertyVariantType>(
                    _bus, service, objMgrPath);
            if (objects.size() > 0)
            {
                useManagedObj = true;
            }
            // insert all objects that are in groups but remove any NaN values
            insertFilteredObjects(objects);
        }
    }

    if (!useManagedObj)
    {
        // No object manager interface provided by service?
        // Or no object is managed?
        // Attempt to retrieve property directly
        try
        {
            auto value =
                util::SDBusPlus::getPropertyVariant<PropertyVariantType>(
                    _bus, service, path, intf, prop);

            setProperty(path, intf, prop, value);
        }
        catch (const std::exception& e)
        {}
        return;
    }
}

const std::optional<PropertyVariantType> Manager::getProperty(
    const std::string& path, const std::string& intf, const std::string& prop)
{
    // TODO Objects hosted by fan control (i.e. ThermalMode) are required to
    // update the cache upon being set/updated
    auto itPath = _objects.find(path);
    if (itPath != _objects.end())
    {
        auto itIntf = itPath->second.find(intf);
        if (itIntf != itPath->second.end())
        {
            auto itProp = itIntf->second.find(prop);
            if (itProp != itIntf->second.end())
            {
                return itProp->second;
            }
        }
    }

    return std::nullopt;
}

void Manager::setProperty(const std::string& path, const std::string& intf,
                          const std::string& prop, PropertyVariantType value)
{
    // filter NaNs out of the cache
    if (PropertyContainsNan(value))
    {
        // dont use operator [] if paths dont exist
        if (_objects.find(path) != _objects.end() &&
            _objects[path].find(intf) != _objects[path].end())
        {
            _objects[path][intf].erase(prop);
        }
    }
    else
    {
        _objects[path][intf][prop] = std::move(value);
    }
}

void Manager::addTimer(const TimerType type,
                       const std::chrono::microseconds interval,
                       std::unique_ptr<TimerPkg> pkg)
{
    auto dataPtr =
        std::make_unique<TimerData>(std::make_pair(type, std::move(*pkg)));
    Timer timer(_event,
                std::bind(&Manager::timerExpired, this, std::ref(*dataPtr)));
    if (type == TimerType::repeating)
    {
        timer.restart(interval);
    }
    else if (type == TimerType::oneshot)
    {
        timer.restartOnce(interval);
    }
    else
    {
        throw std::invalid_argument("Invalid Timer Type");
    }
    _timers.emplace_back(std::move(dataPtr), std::move(timer));
}

void Manager::addGroups(const std::vector<Group>& groups)
{
    std::string lastServ;
    std::vector<std::string> objMgrPaths;
    std::set<std::string> services;
    for (const auto& group : groups)
    {
        for (const auto& member : group.getMembers())
        {
            try
            {
                auto service = group.getService();
                if (service.empty())
                {
                    service = getService(member, group.getInterface());
                }

                if (!service.empty())
                {
                    if (lastServ != service)
                    {
                        objMgrPaths = getPaths(
                            service, "org.freedesktop.DBus.ObjectManager");
                        lastServ = service;
                    }

                    // Look for the ObjectManager as an ancestor from the
                    // member.
                    auto hasObjMgr = std::any_of(
                        objMgrPaths.begin(), objMgrPaths.end(),
                        [&member](const auto& path) {
                            return member.find(path) != std::string::npos;
                        });

                    if (!hasObjMgr)
                    {
                        // No object manager interface provided for group member
                        // Attempt to retrieve group member property directly
                        try
                        {
                            auto value = util::SDBusPlus::getPropertyVariant<
                                PropertyVariantType>(_bus, service, member,
                                                     group.getInterface(),
                                                     group.getProperty());
                            setProperty(member, group.getInterface(),
                                        group.getProperty(), value);
                        }
                        catch (const std::exception& e)
                        {}
                        continue;
                    }

                    if (services.find(service) == services.end())
                    {
                        services.insert(service);
                        for (const auto& objMgrPath : objMgrPaths)
                        {
                            // Get all managed objects from the service
                            auto objects = util::SDBusPlus::getManagedObjects<
                                PropertyVariantType>(_bus, service, objMgrPath);

                            // Insert objects into cache
                            insertFilteredObjects(objects);
                        }
                    }
                }
            }
            catch (const util::DBusError&)
            {
                // No service or property found for group member with the
                // group's configured interface
                continue;
            }
        }
    }
}

void Manager::timerExpired(TimerData& data)
{
    if (std::get<bool>(data.second))
    {
        addGroups(std::get<const std::vector<Group>&>(data.second));
    }

    auto& actions =
        std::get<std::vector<std::unique_ptr<ActionBase>>&>(data.second);
    // Perform the actions in the timer data
    std::for_each(actions.begin(), actions.end(),
                  [](auto& action) { action->run(); });

    // Remove oneshot timers after they expired
    if (data.first == TimerType::oneshot)
    {
        auto itTimer = std::find_if(
            _timers.begin(), _timers.end(), [&data](const auto& timer) {
                return (data.first == timer.first->first &&
                        (std::get<std::string>(data.second) ==
                         std::get<std::string>(timer.first->second)));
            });
        if (itTimer != std::end(_timers))
        {
            _timers.erase(itTimer);
        }
    }
}

void Manager::handleSignal(sdbusplus::message_t& msg,
                           const std::vector<SignalPkg>* pkgs)
{
    for (auto& pkg : *pkgs)
    {
        // Handle the signal callback and only run the actions if the handler
        // updated the cache for the given SignalObject
        if (std::get<SignalHandler>(
                pkg)(msg, std::get<SignalObject>(pkg), *this))
        {
            // Perform the actions in the handler package
            auto& actions = std::get<TriggerActions>(pkg);
            std::for_each(actions.begin(), actions.end(), [](auto& action) {
                if (action.get())
                {
                    action.get()->run();
                }
            });
        }
        // Only rewind message when not last package
        if (&pkg != &pkgs->back())
        {
            sd_bus_message_rewind(msg.get(), true);
        }
    }
}

void Manager::setProfiles()
{
    // Profiles JSON config file is optional
    auto confFile =
        fan::JsonConfig::getConfFile(confAppName, Profile::confFileName, true);

    _profiles.clear();
    if (!confFile.empty())
    {
        for (const auto& entry : fan::JsonConfig::load(confFile))
        {
            auto obj = std::make_unique<Profile>(entry);
            _profiles.emplace(
                std::make_pair(obj->getName(), obj->getProfiles()),
                std::move(obj));
        }
    }

    // Ensure all configurations use the same set of active profiles
    // (In case a profile's active state changes during configuration)
    _activeProfiles.clear();
    for (const auto& profile : _profiles)
    {
        if (profile.second->isActive())
        {
            _activeProfiles.emplace_back(profile.first.first);
        }
    }
}

void Manager::addParameterTrigger(
    const std::string& name, std::vector<std::unique_ptr<ActionBase>>& actions)
{
    auto it = _parameterTriggers.find(name);
    if (it != _parameterTriggers.end())
    {
        std::for_each(actions.begin(), actions.end(),
                      [&actList = it->second](auto& action) {
                          actList.emplace_back(std::ref(action));
                      });
    }
    else
    {
        TriggerActions triggerActions;
        std::for_each(actions.begin(), actions.end(),
                      [&triggerActions](auto& action) {
                          triggerActions.emplace_back(std::ref(action));
                      });
        _parameterTriggers[name] = std::move(triggerActions);
    }
}

void Manager::runParameterActions(const std::string& name)
{
    auto it = _parameterTriggers.find(name);
    if (it != _parameterTriggers.end())
    {
        std::for_each(it->second.begin(), it->second.end(),
                      [](auto& action) { action.get()->run(); });
    }
}

} // namespace phosphor::fan::control::json
