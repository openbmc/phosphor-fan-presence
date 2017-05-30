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
#include <algorithm>
#include <phosphor-logging/log.hpp>
#include <unistd.h>
#include "manager.hpp"
#include "elog-errors.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{

using namespace phosphor::logging;

constexpr auto SYSTEMD_SERVICE   = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH  = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";
constexpr auto FAN_CONTROL_READY_TARGET = "obmc-fan-control-ready@0.target";

//Note: Future code will check 'mode' before starting control algorithm
Manager::Manager(sdbusplus::bus::bus& bus,
                 Mode mode) :
    _bus(bus)
{
    //Create the appropriate Zone objects based on the
    //actual system configuration.

    //Find the 1 ZoneGroup that meets all of its conditions
    for (auto& group : _zoneLayouts)
    {
        auto& conditions = std::get<conditionListPos>(group);

        if (std::all_of(conditions.begin(), conditions.end(),
                        [](const auto& c)
                        {
                            //TODO: openbmc/openbmc#1500
                            //Still need to actually evaluate the conditions
                            return true;
                        }))
        {
            //Create a Zone object for each zone in this group
            auto& zones = std::get<zoneListPos>(group);

            for (auto& z : zones)
            {
                _zones.emplace(std::get<zoneNumPos>(z),
                               std::make_unique<Zone>(_bus, z));
            }

            break;
        }
    }

}


void Manager::doInit()
{
    for (auto& z: _zones)
    {
        z.second->setFullSpeed();
    }

    auto delay = _powerOnDelay;
    while (delay > 0)
    {
        delay = sleep(delay);
    }

    startFanControlReadyTarget();
}


void Manager::startFanControlReadyTarget()
{
    auto method = _bus.new_method_call(SYSTEMD_SERVICE,
            SYSTEMD_OBJ_PATH,
            SYSTEMD_INTERFACE,
            "StartUnit");

    method.append(FAN_CONTROL_READY_TARGET);
    method.append("replace");

    auto response = _bus.call(method);
    if (response.is_method_error())
    {
        elog<xyz::openbmc_project::Common::Fan::InternalFailure>(
            phosphor::logging::xyz::openbmc_project::Common::Fan
                ::InternalFailure::MESSAGE("Failed to start fan control ready target"));
    }
}

}
}
}
