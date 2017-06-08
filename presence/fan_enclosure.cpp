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
#include "fan_enclosure.hpp"
#include "sdbusplus.hpp"
#include "utility.hpp"

namespace phosphor
{
namespace fan
{
namespace presence
{

using namespace phosphor::logging;
using namespace std::literals::string_literals;

//TODO Should get these from phosphor-inventory-manager config.h
const auto INVENTORY_PATH = "/xyz/openbmc_project/inventory"s;
const auto INVENTORY_INTF = "xyz.openbmc_project.Inventory.Manager"s;

presenceState FanEnclosure::getCurPresState()
{
    auto presPred = [](auto const& s) {return s->isPresent();};
    // Determine if all sensors show fan is not present
    auto isPresent = std::any_of(sensors.begin(),
                                 sensors.end(),
                                 presPred);

    return (isPresent) ? PRESENT : NOT_PRESENT;
}

FanEnclosure::ObjectMap FanEnclosure::getObjectMap(const bool curPresState)
{
    ObjectMap invObj;
    InterfaceMap invIntf;
    PropertyMap invProp;

    invProp.emplace("Present", curPresState);
    invProp.emplace("PrettyName", fanDesc);
    invIntf.emplace("xyz.openbmc_project.Inventory.Item", std::move(invProp));
    Object fanInvPath = invPath;
    invObj.emplace(std::move(fanInvPath), std::move(invIntf));

    return invObj;
}

void FanEnclosure::updInventory()
{
    auto curPresState = getCurPresState();
    // Only update inventory when presence state changed
    if (presState != curPresState)
    {
        // Update inventory for this fan
        util::SDBusPlus::lookupAndCallMethod(
                INVENTORY_PATH,
                INVENTORY_INTF,
                "Notify"s,
                getObjectMap(curPresState));
        // Inventory updated, set presence state to current
        presState = curPresState;
    }
}

void FanEnclosure::addSensor(
    std::unique_ptr<Sensor>&& sensor)
{
    FanEnclosure::sensors.push_back(std::move(sensor));
}

} // namespace presence
} // namespace fan
} // namespace phosphor
