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
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include "zone.hpp"
#include "utility.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{

using namespace phosphor::logging;
using InternalFailure = sdbusplus::xyz::openbmc_project::Common::
                             Error::InternalFailure;

Zone::Zone(Mode mode,
           sdbusplus::bus::bus& bus,
           const ZoneDefinition& def) :
    _bus(bus),
    _fullSpeed(std::get<fullSpeedPos>(def)),
    _zoneNum(std::get<zoneNumPos>(def)),
    _defFloorSpeed(std::get<floorSpeedPos>(def)),
    _defCeilingSpeed(std::get<fullSpeedPos>(def))
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
    }
}


void Zone::setSpeed(uint64_t speed)
{
    for (auto& fan : _fans)
    {
        //TODO openbmc/openbmc#1626 Move to control algorithm function
        if (speed < _floorSpeed)
        {
            speed = _floorSpeed;
        }
        //TODO openbmc/openbmc#1626 Move to control algorithm function
        if (speed > _ceilingSpeed)
        {
            speed = _ceilingSpeed;
        }
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
        log<level::ERR>("Error in host call response for retrieving property");
        elog<InternalFailure>();
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
