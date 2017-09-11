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
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include "zone.hpp"
#include "utility.hpp"
#include "sdbusplus.hpp"

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
           phosphor::fan::event::EventPtr& events,
           const ZoneDefinition& def) :
    _bus(bus),
    _fullSpeed(std::get<fullSpeedPos>(def)),
    _zoneNum(std::get<zoneNumPos>(def)),
    _defFloorSpeed(std::get<floorSpeedPos>(def)),
    _defCeilingSpeed(std::get<fullSpeedPos>(def)),
    _incDelay(std::get<incDelayPos>(def)),
    _decInterval(std::get<decIntervalPos>(def)),
    _incTimer(events, [this](){ this->incTimerExpired(); }),
    _decTimer(events, [this](){ this->decTimerExpired(); }),
    _sdEvents(events)
{
    auto& fanDefs = std::get<fanListPos>(def);

    for (auto& def : fanDefs)
    {
        _fans.emplace_back(std::make_unique<Fan>(bus, def));
    }

    // Do not enable set speed events when in init mode
    if (mode != Mode::init)
    {
        // Setup signal trigger for set speed events
        for (auto& event : std::get<setSpeedEventsPos>(def))
        {
            initEvent(event);
        }
        // Start timer for fan speed decreases
        if (!_decTimer.running() && _decInterval != seconds::zero())
        {
            _decTimer.start(_decInterval,
                            util::Timer::TimerType::repeating);
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

void Zone::setFloor(uint64_t speed)
{
    _floorSpeed = speed;
    // Floor speed above target, update target to floor speed
    if (_targetSpeed < _floorSpeed)
    {
        requestSpeedIncrease(_floorSpeed - _targetSpeed);
    }
}

void Zone::requestSpeedIncrease(uint64_t targetDelta)
{
    // Only increase speed when delta is higher than
    // the current increase delta for the zone and currently under ceiling
    if (targetDelta > _incSpeedDelta &&
        _targetSpeed < _ceilingSpeed)
    {
        auto requestTarget = _targetSpeed;
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
        _incTimer.start(_incDelay,
                        util::Timer::TimerType::oneshot);
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
    // Only decrease speeds when no requested increases exist and
    // the increase timer is not running (i.e. not in the middle of increasing)
    if (_incSpeedDelta == 0 && !_incTimer.running())
    {
        auto requestTarget = _targetSpeed;
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
    // Get the current value for each property
    for (auto& group : std::get<groupPos>(event))
    {
        try
        {
            refreshProperty(_bus,
                            group.first,
                            std::get<intfPos>(group.second),
                            std::get<propPos>(group.second));
        }
        catch (const InternalFailure& ife)
        {
            log<level::INFO>(
                "Unable to find property",
                entry("PATH=%s", group.first.c_str()),
                entry("INTERFACE=%s", std::get<intfPos>(group.second).c_str()),
                entry("PROPERTY=%s", std::get<propPos>(group.second).c_str()));
        }
    }
    // Setup signal matches for property change events
    for (auto& prop : std::get<propChangeListPos>(event))
    {
        std::unique_ptr<EventData> eventData =
            std::make_unique<EventData>(
                EventData
                {
                    std::get<groupPos>(event),
                    std::get<handlerObjPos>(prop),
                    std::get<actionsPos>(event)
                }
            );
        std::unique_ptr<sdbusplus::server::match::match> match =
            std::make_unique<sdbusplus::server::match::match>(
                _bus,
                std::get<signaturePos>(prop).c_str(),
                std::bind(std::mem_fn(&Zone::handleEvent),
                          this,
                          std::placeholders::_1,
                          eventData.get())
            );
        _signalEvents.emplace_back(std::move(eventData), std::move(match));
    }
    // Attach a timer to run the action of an event
    auto eventTimer = std::get<timerPos>(event);
    if (std::get<intervalPos>(eventTimer) != seconds(0))
    {
        std::unique_ptr<util::Timer> timer =
            std::make_unique<util::Timer>(
                _sdEvents,
                [this,
                 action = &(std::get<actionsPos>(event)),
                 group = &(std::get<groupPos>(event))]()
                 {
                     this->timerExpired(*group, *action);
                 });
        if (!timer->running())
        {
            timer->start(std::get<intervalPos>(eventTimer),
                         util::Timer::TimerType::repeating);
        }
        _timerEvents.emplace_back(std::move(timer));
    }
    // Run action functions for initial event state
    std::for_each(
        std::get<actionsPos>(event).begin(),
        std::get<actionsPos>(event).end(),
        [this, &event](auto const& action)
        {
            action(*this,
                   std::get<groupPos>(event));
        });
}

void Zone::removeEvent(const SetSpeedEvent& event)
{
    // Find the signal event to be removed
    auto it = std::find_if(
        _signalEvents.begin(),
        _signalEvents.end(),
        [&event](auto const& se)
        {
            auto seEventData = *std::get<signalEventDataPos>(se);
            if (std::get<eventActionsPos>(seEventData).size() !=
                std::get<actionsPos>(event).size())
            {
                return false;
            }
            else
            {
                // TODO openbmc/openbmc#2328 - Use the action function target
                // for comparison
                auto actsEqual = [](auto const& a1,
                                    auto const& a2)
                        {
                            return a1.target_type().name() ==
                                   a2.target_type().name();
                        };
                return
                (
                    std::get<eventGroupPos>(seEventData) ==
                        std::get<groupPos>(event) &&
                    std::equal(std::get<actionsPos>(event).begin(),
                               std::get<actionsPos>(event).end(),
                               std::get<eventActionsPos>(seEventData).begin(),
                               actsEqual)
                );
            }
        });
    if (it != std::end(_signalEvents))
    {
        std::get<signalEventDataPos>(*it).reset();
        std::get<signalMatchPos>(*it).reset();
        _signalEvents.erase(it);
    }
}

void Zone::refreshProperty(sdbusplus::bus::bus& bus,
                           const std::string& path,
                           const std::string& iface,
                           const std::string& prop)
{
    PropertyVariantType property;
    getProperty(_bus, path, iface, prop, property);
    setPropertyValue(path.c_str(), iface.c_str(), prop.c_str(), property);
}

void Zone::getProperty(sdbusplus::bus::bus& bus,
                       const std::string& path,
                       const std::string& iface,
                       const std::string& prop,
                       PropertyVariantType& value)
{
    auto serv = util::SDBusPlus::getService(bus, path, iface);
    auto hostCall = bus.new_method_call(serv.c_str(),
                                        path.c_str(),
                                        "org.freedesktop.DBus.Properties",
                                        "Get");
    hostCall.append(iface);
    hostCall.append(prop);
    auto hostResponseMsg = bus.call(hostCall);
    if (hostResponseMsg.is_method_error())
    {
        log<level::INFO>("Host call response error for retrieving property");
        elog<InternalFailure>();
    }
    hostResponseMsg.read(value);
}

void Zone::timerExpired(Group eventGroup, std::vector<Action> eventActions)
{
    // Perform the actions
    std::for_each(eventActions.begin(),
                  eventActions.end(),
                  [this, &eventGroup](auto const& action)
                  {
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
            action(*this,
                   std::get<eventGroupPos>(*eventData));
        });
}

}
}
}
