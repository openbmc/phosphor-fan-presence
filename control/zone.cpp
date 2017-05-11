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
#include "zone.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{


Zone::Zone(sdbusplus::bus::bus& bus,
           const ZoneDefinition& def) :
    _bus(bus),
    _fullSpeed(std::get<fullSpeedPos>(def)),
    _zoneNum(std::get<zoneNumPos>(def))
{
    auto& fanDefs = std::get<fanListPos>(def);

    for (auto& def : fanDefs)
    {
        _fans.emplace_back(std::make_unique<Fan>(bus, def));
    }

    // Setup signal trigger for property changes
    for (auto& event : std::get<setSpeedEventsPos>(def))
    {
        for (auto& prop : event)
        {
            _signalEvents.emplace_back(
                std::make_unique<SignalEvent>(this,
                                              std::get<handlerObjPos>(prop)
                                              ));
            _matches.emplace_back(bus,
                                  std::get<signaturePos>(prop).c_str(),
                                  signalHandler,
                                  _signalEvents.back().get());
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

int Zone::signalHandler(sd_bus_message* msg,
                        void* data,
                        sd_bus_error* err)
{
    auto sdbpMsg = sdbusplus::message::message(msg);
    auto& signalEvent = *static_cast<SignalEvent*>(data);
    std::get<zoneObjPos>(signalEvent)->handleEvent(
        sdbpMsg,
        std::get<handlerObjPos>(signalEvent));
    return 0;
}

void Zone::handleEvent(sdbusplus::message::message& msg,
                       const Handler& handler)
{
    // Handle the callback
    handler(_bus, msg, *this);
}


}
}
}
