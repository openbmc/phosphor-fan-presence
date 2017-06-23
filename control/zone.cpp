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
#include <phosphor-logging/elog.hpp>
#include "zone.hpp"
#include "utility.hpp"
#include <iostream>

namespace phosphor
{
namespace fan
{
namespace control
{

using namespace phosphor::logging;
using namespace std::chrono;

Zone::Zone(Mode mode,
           sdbusplus::bus::bus& bus,
           phosphor::fan::event::EventPtr& events,
           const ZoneDefinition& def) :
    _bus(bus),
    _fullSpeed(std::get<fullSpeedPos>(def)),
    _zoneNum(std::get<zoneNumPos>(def)),
    _defFloorSpeed(std::get<floorSpeedPos>(def)),
    _defCeilingSpeed(std::get<fullSpeedPos>(def)),
    _decTimer(events, [this](){ this->decTimerExpired(); })
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
            // Get the current value for each property
            for (auto& entry : std::get<groupPos>(event))
            {
                try
                {
                    PropertyVariantType property;
                    getProperty(_bus,
                                entry.first,
                                std::get<intfPos>(entry.second),
                                std::get<propPos>(entry.second),
                                property);
                    setPropertyValue(entry.first.c_str(),
                                     std::get<intfPos>(entry.second).c_str(),
                                     std::get<propPos>(entry.second).c_str(),
                                     property);
                }
                catch (const std::exception& e)
                {
                    log<level::ERR>(e.what());
                }
            }
            // Setup signal matches for property change events
            for (auto& prop : std::get<propChangeListPos>(event))
            {
                _signalEvents.emplace_back(
                        std::make_unique<EventData>(
                                EventData
                                {
                                    std::get<groupPos>(event),
                                    std::get<handlerObjPos>(prop),
                                    std::get<actionPos>(event)
                                }));
                _matches.emplace_back(
                        bus,
                        std::get<signaturePos>(prop).c_str(),
                        std::bind(std::mem_fn(&Zone::handleEvent),
                                  this,
                                  std::placeholders::_1,
                                  _signalEvents.back().get()));
            }
            // Run action function for initial event state
            std::get<actionPos>(event)(*this,
                                       std::get<groupPos>(event));
        }
        // Start timer for fan speed decreases
        if (!_decTimer.running())
        {
            //TODO Update time value to what's given in zones yaml
            _decTimer.start(duration_cast<microseconds>(seconds(30)),
                            phosphor::fan::util::Timer::TimerType::repeating);
        }
    }
}


void Zone::setSpeed(uint64_t speed)
{
    for (auto& fan : _fans)
    {
        fan->setSpeed(speed);
    }
}

void Zone::setActiveAllow(const Group* group, bool isActiveAllow)
{
    _active[group] = isActiveAllow;
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

void Zone::requestSpeedIncrease(uint64_t targetDelta)
{
    // Only increase speed when delta is higher than
    // the current increase delta for the zone and currently under ceiling
    if (targetDelta > _incSpeedDelta &&
        _targetSpeed < _ceilingSpeed)
    {
        _targetSpeed = (targetDelta - _incSpeedDelta) + _targetSpeed;
        _incSpeedDelta = targetDelta;
        //TODO openbmc/openbmc#1625 Cancel current timer countdown
        //TODO Floor speed above target, update target to floor speed
        if (_targetSpeed < _floorSpeed)
        {
            _targetSpeed = _floorSpeed;
        }
        // Target speed can not go above a defined ceiling speed
        if (_targetSpeed > _ceilingSpeed)
        {
            _targetSpeed = _ceilingSpeed;
        }

        setSpeed(_targetSpeed);
        //TODO openbmc/openbmc#1625 Start timer countdown for fan speed increase
    }
    //TODO openbmc/openbmc#1625 Clear increase delta when timer expires
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
    auto now = steady_clock::now().time_since_epoch();
    std::cout <<
        "***decTimerExpired (" <<
        duration_cast<microseconds>(now).count() <<
        ")***" << std::endl;

    // Only decrease speeds when no requested increases exist
    //TODO Add increase timer not running (i.e. not in the middle of increasing)
    if (_incSpeedDelta == 0)
    {
        auto currentSpeed = _targetSpeed;
        _targetSpeed = _targetSpeed - _decSpeedDelta;
        // Target speed wrapped and can not go below the defined floor speed
        if (_targetSpeed > currentSpeed || _targetSpeed < _floorSpeed)
        {
            _targetSpeed = _floorSpeed;
        }
        setSpeed(_targetSpeed);
    }
    // Clear decrease delta when timer expires
    _decSpeedDelta = 0;
    // Decrease timer is restarted since its repeating
}

void Zone::getProperty(sdbusplus::bus::bus& bus,
                       const std::string& path,
                       const std::string& iface,
                       const std::string& prop,
                       PropertyVariantType& value)
{
    auto serv = phosphor::fan::util::getService(path, iface, bus);
    auto hostCall = bus.new_method_call(serv.c_str(),
                                        path.c_str(),
                                        "org.freedesktop.DBus.Properties",
                                        "Get");
    hostCall.append(iface);
    hostCall.append(prop);
    auto hostResponseMsg = bus.call(hostCall);
    if (hostResponseMsg.is_method_error())
    {
        throw std::runtime_error(
            "Error in host call response for retrieving property");
    }
    hostResponseMsg.read(value);
}

void Zone::handleEvent(sdbusplus::message::message& msg,
                       const EventData* eventData)
{
    // Handle the callback
    std::get<eventHandlerPos>(*eventData)(_bus, msg, *this);
    // Perform the action
    std::get<eventActionPos>(*eventData)(*this,
                                         std::get<eventGroupPos>(*eventData));
}

}
}
}
