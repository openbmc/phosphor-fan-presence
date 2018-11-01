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
#include <chrono>
#include <functional>
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include "zone.hpp"
#include "utility.hpp"
#include "sdbusplus.hpp"

#include "triggers.hpp"
#include <iostream>

namespace phosphor
{
namespace fan
{
namespace control
{

using namespace std::chrono;
using namespace phosphor::fan;
using namespace phosphor::logging;
using InternalFailure = sdbusplus::xyz::openbmc_project::Common::
                             Error::InternalFailure;

Zone::Zone(Mode mode,
           sdbusplus::bus::bus& bus,
           const sdeventplus::Event& event,
           const ZoneDefinition& def) :
    _bus(bus),
    _fullSpeed(std::get<fullSpeedPos>(def)),
    _zoneNum(std::get<zoneNumPos>(def)),
    _defFloorSpeed(std::get<floorSpeedPos>(def)),
    _defCeilingSpeed(std::get<fullSpeedPos>(def)),
    _incDelay(std::get<incDelayPos>(def)),
    _decInterval(std::get<decIntervalPos>(def)),
    _incTimer(event, std::bind(&Zone::incTimerExpired, this)),
    _decTimer(event, std::bind(&Zone::decTimerExpired, this)),
    _eventLoop(event)
{
    auto& fanDefs = std::get<fanListPos>(def);

    for (auto& def : fanDefs)
    {
        _fans.emplace_back(std::make_unique<Fan>(bus, def));
    }

    // Do not enable set speed events when in init mode
    if (mode != Mode::init)
    {
        // Update target speed to current zone target speed
        if (!_fans.empty())
        {
            _targetSpeed = _fans.front()->getTargetSpeed();
        }
        // Setup signal trigger for set speed events
        for (auto& event : std::get<setSpeedEventsPos>(def))
        {
            initEvent(event);
        }
        // Start timer for fan speed decreases
        if (!_decTimer.running() && _decInterval != seconds::zero())
        {
            _decTimer.start(_decInterval, TimerType::repeating);
        }
    }
}

void Zone::setSpeed(uint64_t speed)
{
    if (_isActive)
    {
        _targetSpeed = speed;
        for (auto& fan : _fans)
        {
            fan->setSpeed(_targetSpeed);
        }
    }
}

void Zone::setFullSpeed()
{
    if (_fullSpeed != 0)
    {
        _targetSpeed = _fullSpeed;
        for (auto& fan : _fans)
        {
            fan->setSpeed(_targetSpeed);
        }
    }
}

void Zone::setActiveAllow(const Group* group, bool isActiveAllow)
{
    _active[*(group)] = isActiveAllow;
    if (!isActiveAllow)
    {
        _isActive = false;
    }
    else
    {
        // Check all entries are set to allow control active
        auto actPred = [](auto const& entry) {return entry.second;};
        _isActive = std::all_of(_active.begin(),
                                _active.end(),
                                actPred);
    }
}

void Zone::removeService(const Group* group,
                         const std::string& name)
{
    try
    {
        auto& sNames = _services.at(*group);
        auto it = std::find_if(
            sNames.begin(),
            sNames.end(),
            [&name](auto const& entry)
            {
                return name == std::get<namePos>(entry);
            }
        );
        if (it != std::end(sNames))
        {
            // Remove service name from group
            sNames.erase(it);
        }
    }
    catch (const std::out_of_range& oore)
    {
        // No services for group found
    }
}

void Zone::setServiceOwner(const Group* group,
                           const std::string& name,
                           const bool hasOwner)
{
    try
    {
        auto& sNames = _services.at(*group);
        auto it = std::find_if(
            sNames.begin(),
            sNames.end(),
            [&name](auto const& entry)
            {
                return name == std::get<namePos>(entry);
            }
        );
        if (it != std::end(sNames))
        {
            std::get<hasOwnerPos>(*it) = hasOwner;
        }
        else
        {
            _services[*group].emplace_back(name, hasOwner);
        }
    }
    catch (const std::out_of_range& oore)
    {
        _services[*group].emplace_back(name, hasOwner);
    }
}

void Zone::setServices(const Group* group)
{
    // Remove the empty service name if exists
    removeService(group, "");
    for (auto it = group->begin(); it != group->end(); ++it)
    {
        std::string name;
        bool hasOwner = false;
        try
        {
            name = getService(std::get<pathPos>(*it),
                              std::get<intfPos>(*it));
            hasOwner = util::SDBusPlus::callMethodAndRead<bool>(
                    _bus,
                    "org.freedesktop.DBus",
                    "/org/freedesktop/DBus",
                    "org.freedesktop.DBus",
                    "NameHasOwner",
                    name);
        }
        catch (const util::DBusMethodError& e)
        {
            // Failed to get service name owner state
            hasOwner = false;
        }
        setServiceOwner(group, name, hasOwner);
    }
}

void Zone::setFloor(uint64_t speed)
{
    // Check all entries are set to allow floor to be set
    auto pred = [](auto const& entry) {return entry.second;};
    auto setFloor = std::all_of(_floorChange.begin(),
                                _floorChange.end(),
                                pred);
    if (setFloor)
    {
        _floorSpeed = speed;
        // Floor speed above target, update target to floor speed
        if (_targetSpeed < _floorSpeed)
        {
            requestSpeedIncrease(_floorSpeed - _targetSpeed);
        }
    }
}

void Zone::requestSpeedIncrease(uint64_t targetDelta)
{
    // Only increase speed when delta is higher than
    // the current increase delta for the zone and currently under ceiling
    if (targetDelta > _incSpeedDelta &&
        _targetSpeed < _ceilingSpeed)
    {
        auto requestTarget = getRequestSpeedBase();
        requestTarget = (targetDelta - _incSpeedDelta) + requestTarget;
        _incSpeedDelta = targetDelta;
        // Target speed can not go above a defined ceiling speed
        if (requestTarget > _ceilingSpeed)
        {
            requestTarget = _ceilingSpeed;
        }
        // Cancel current timer countdown
        if (_incTimer.running())
        {
            _incTimer.stop();
        }
        setSpeed(requestTarget);
        // Start timer countdown for fan speed increase
        _incTimer.start(_incDelay, TimerType::oneshot);
    }
}

void Zone::incTimerExpired()
{
    // Clear increase delta when timer expires allowing additional speed
    // increase requests or speed decreases to occur
    _incSpeedDelta = 0;
}

void Zone::requestSpeedDecrease(uint64_t targetDelta)
{
    // Only decrease the lowest target delta requested
    if (_decSpeedDelta == 0 || targetDelta < _decSpeedDelta)
    {
        _decSpeedDelta = targetDelta;
    }
}

void Zone::decTimerExpired()
{
    // Check all entries are set to allow a decrease
    auto pred = [](auto const& entry) {return entry.second;};
    auto decAllowed = std::all_of(_decAllowed.begin(),
                                  _decAllowed.end(),
                                  pred);

    // Only decrease speeds when allowed,
    // where no requested increases exist and
    // the increase timer is not running
    // (i.e. not in the middle of increasing)
    if (decAllowed && _incSpeedDelta == 0 && !_incTimer.running())
    {
        auto requestTarget = getRequestSpeedBase();
        // Request target speed should not start above ceiling
        if (requestTarget > _ceilingSpeed)
        {
            requestTarget = _ceilingSpeed;
        }
        // Target speed can not go below the defined floor speed
        if ((requestTarget < _decSpeedDelta) ||
            (requestTarget - _decSpeedDelta < _floorSpeed))
        {
            requestTarget = _floorSpeed;
        }
        else
        {
            requestTarget = requestTarget - _decSpeedDelta;
        }
        setSpeed(requestTarget);
    }
    // Clear decrease delta when timer expires
    _decSpeedDelta = 0;
    // Decrease timer is restarted since its repeating
}

void Zone::initEvent(const SetSpeedEvent& event)
{
    // Enable event triggers
    std::for_each(
        std::get<triggerPos>(event).begin(),
        std::get<triggerPos>(event).end(),
        [this, &event](auto const& trigger)
        {
            trigger(*this,
                    std::get<groupPos>(event),
                    std::get<actionsPos>(event));
        }
    );
}

void Zone::removeEvent(const SetSpeedEvent& event)
{
    // Remove triggers of the event
    for (auto& trig : std::get<triggerPos>(event))
    {
        std::cout << "Trigger target_type name: " << trig.target_type().name() << std::endl;
        // void (*const* ptr)(Zone&, const Group&, const std::vector<Action>&) =
        //     trig.target<void(*)(Zone&, const Group&, const std::vector<Action>&)>();
        // if (ptr)// && *ptr == Trigger(trigger::signal))
        // {
            auto it = findSignal(trig,
                                 std::get<groupPos>(event),
                                 std::get<actionsPos>(event));
            if (it != std::end(getSignalEvents()))
            {
                removeSignal(it);
            }
        // }
    }
    // Remove timers of the event
    // if (std::get<intervalPos>(std::get<timerConfPos>(event)) != seconds(0))
    // {
        auto it = findTimer(std::get<groupPos>(event),
                            std::get<actionsPos>(event));
        if (it != std::end(getTimerEvents()))
        {
            removeTimer(it);
        }
    // }
}

std::vector<SignalEvent>::iterator Zone::findSignal(
        const Trigger& signal,
        const Group& eGroup,
        const std::vector<Action>& eActions)
{
    // Find the signal in the event to be removed
    for (auto it = _signalEvents.begin(); it != _signalEvents.end(); ++ it)
    {
        const auto& seEventData = *std::get<signalEventDataPos>(*it);
        if (eGroup == std::get<eventGroupPos>(seEventData) &&
            eActions.size() == std::get<eventActionsPos>(seEventData).size())
        {
            // TODO openbmc/openbmc#2328 - Use the function target
            // for comparison
            auto actsEqual = [](auto const& a1,
                                auto const& a2)
                    {
                        return a1.target_type().name() ==
                               a2.target_type().name();
                    };
            if (std::equal(eActions.begin(),
                           eActions.end(),
                           std::get<eventActionsPos>(seEventData).begin(),
                           actsEqual))
            {
                return it;
            }
        }
    }

    return _signalEvents.end();
}

std::vector<TimerEvent>::iterator Zone::findTimer(
        const Group& eventGroup,
        const std::vector<Action>& eventActions)
{
    for (auto it = _timerEvents.begin(); it != _timerEvents.end(); ++it)
    {
        const auto& teEventData = *std::get<timerEventDataPos>(*it);
        if (std::get<eventActionsPos>(teEventData).size() ==
            eventActions.size())
        {
            // TODO openbmc/openbmc#2328 - Use the action function target
            // for comparison
            auto actsEqual = [](auto const& a1,
                                auto const& a2)
                    {
                        return a1.target_type().name() ==
                               a2.target_type().name();
                    };
            if (std::get<eventGroupPos>(teEventData) == eventGroup &&
                std::equal(eventActions.begin(),
                           eventActions.end(),
                           std::get<eventActionsPos>(teEventData).begin(),
                           actsEqual))
            {
                return it;
            }
        }
    }

    return _timerEvents.end();
}

void Zone::addTimer(const Group& group,
                    const std::vector<Action>& actions,
                    const TimerConf& tConf)
{
    // Associate event data with timer
    auto data = std::make_unique<EventData>(
            group,
            "",
            nullptr,
            actions
    );
    auto timer = std::make_unique<util::Timer>(
        _eventLoop,
        std::bind(&Zone::timerExpired,
                  this,
                  std::cref(std::get<Group>(*data)),
                  std::cref(std::get<std::vector<Action>>(*data)))
    );
    if (!timer->running())
    {
        timer->start(std::get<intervalPos>(tConf),
                     std::get<typePos>(tConf));
    }
    _timerEvents.emplace_back(std::move(data), std::move(timer));
}

void Zone::timerExpired(const Group& eventGroup,
                        const std::vector<Action>& eventActions)
{
    // Perform the actions
    std::for_each(eventActions.begin(),
                  eventActions.end(),
                  [this, &eventGroup](auto const& action)
                  {
                      // TODO Use action groups instead of event groups if exists
                      action(*this, eventGroup);
                  });
}

void Zone::handleEvent(sdbusplus::message::message& msg,
                       const EventData* eventData)
{
    // Handle the callback
    std::get<eventHandlerPos>(*eventData)(_bus, msg, *this);
    // Perform the actions
    std::for_each(
        std::get<eventActionsPos>(*eventData).begin(),
        std::get<eventActionsPos>(*eventData).end(),
        [this, &eventData](auto const& action)
        {
            // TODO Use action groups instead of event groups if exists
            action(*this,
                   std::get<eventGroupPos>(*eventData));
        });
}

const std::string& Zone::getService(const std::string& path,
                                    const std::string& intf)
{
    // Retrieve service from cache
    auto srvIter = _servTree.find(path);
    if (srvIter != _servTree.end())
    {
        for (auto& serv : srvIter->second)
        {
            auto it = std::find_if(
                serv.second.begin(),
                serv.second.end(),
                [&intf](auto const& interface)
                {
                    return intf == interface;
                });
            if (it != std::end(serv.second))
            {
                // Service found
                return serv.first;
            }
        }
        // Interface not found in cache, add and return
        return addServices(path, intf, 0);
    }
    else
    {
        // Path not found in cache, add and return
        return addServices(path, intf, 0);
    }
}

const std::string& Zone::addServices(const std::string& path,
                                     const std::string& intf,
                                     int32_t depth)
{
    static const std::string empty = "";
    auto it = _servTree.end();

    // Get all subtree objects for the given interface
    auto objects = util::SDBusPlus::getSubTree(_bus, "/", intf, depth);
    // Add what's returned to the cache of path->services
    for (auto& pIter : objects)
    {
        auto pathIter = _servTree.find(pIter.first);
        if (pathIter != _servTree.end())
        {
            // Path found in cache
            for (auto& sIter : pIter.second)
            {
                auto servIter = pathIter->second.find(sIter.first);
                if (servIter != pathIter->second.end())
                {
                    // Service found in cache
                    for (auto& iIter : sIter.second)
                    {
                        if (std::find(servIter->second.begin(),
                                      servIter->second.end(),
                                      iIter) == servIter->second.end())
                        {
                            // Add interface to cache
                            servIter->second.emplace_back(iIter);
                        }
                    }
                }
                else
                {
                    // Service not found in cache
                    pathIter->second.insert(sIter);
                }
            }
        }
        else
        {
            _servTree.insert(pIter);
        }
        // When the paths match, since a single interface constraint is given,
        // that is the service to return
        if (path == pIter.first)
        {
            it = _servTree.find(pIter.first);
        }
    }

    if (it != _servTree.end())
    {
        return it->second.begin()->first;
    }

    return empty;
}

}
}
}
