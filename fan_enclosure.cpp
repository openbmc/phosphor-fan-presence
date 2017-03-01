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


namespace phosphor
{
namespace fan
{
namespace presence
{

using namespace phosphor::logging;

//TODO Should get these from phosphor-objmgr config.h
constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/ObjectMapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";

//TODO Should get these from phosphor-inventory-manager config.h
constexpr auto INVENTORY_PATH = "/xyz/openbmc_project/inventory";
constexpr auto INVENTORY_INTF = "xyz.openbmc_project.Inventory.Manager";

bool FanEnclosure::getCurPresState()
{
    auto presPred = [](auto const& s) {return s->isPresent();};
    // Determine if all sensors show fan is not present
    auto isPresent = std::any_of(sensors.begin(),
                                 sensors.end(),
                                 presPred);

    return isPresent;
}

ObjectMap FanEnclosure::getObjectMap(const bool& curPresState)
{
    ObjectMap invObj;
    InterfaceMap invIntf;
    PropertyMap invProp;

    invProp.emplace("Present", curPresState);
    invProp.emplace("PrettyName", fanDesc);
    invIntf.emplace("xyz.openbmc_project.Inventory.Item", invProp);
    Object fanInvPath = invPath;
    invObj.emplace(fanInvPath, invIntf);

    return invObj;
}

std::string FanEnclosure::getInvService()
{
    auto mapperCall = bus.new_method_call(MAPPER_BUSNAME,
                                          MAPPER_PATH,
                                          MAPPER_INTERFACE,
                                          "GetObject");

    mapperCall.append(INVENTORY_PATH);
    mapperCall.append(std::vector<std::string>({INVENTORY_INTF}));

    auto mapperResponseMsg = bus.call(mapperCall);
    if (mapperResponseMsg.is_method_error())
    {
        throw std::runtime_error(
            "Error in mapper call to get inventory service name");
    }

    std::map<std::string, std::vector<std::string>> mapperResponse;
    mapperResponseMsg.read(mapperResponse);

    if (mapperResponse.empty())
    {
        throw std::runtime_error(
            "Error in mapper response for inventory service name");
    }

    return mapperResponse.begin()->first;
}

void FanEnclosure::updInventory()
{
    auto curPresState = getCurPresState();
    // Only update inventory when presence state changed
    if (presState != curPresState)
    {
        // Get inventory object for this fan
        ObjectMap invObj = getObjectMap(curPresState);
        // Get inventory manager service name from mapper
        std::string invService;
        try
        {
            invService = getInvService();
        }
        catch (const std::runtime_error& err)
        {
            log<level::ERR>(err.what());
            return;
        }
        // Update inventory for this fan
        auto invMsg = bus.new_method_call(invService.c_str(),
                                          INVENTORY_PATH,
                                          INVENTORY_INTF,
                                          "Notify");
        invMsg.append(std::move(invObj));
        auto invMgrResponseMsg = bus.call(invMsg);
        if (invMgrResponseMsg.is_method_error())
        {
            log<level::ERR>(
                "Error in inventory manager call to update inventory");
            return;
        }
        // Inventory updated, set presence state to current
        presState = curPresState;
    }
}

void FanEnclosure::addSensor(
    std::unique_ptr<phosphor::fan::presence::Sensor>&& sensor)
{
    FanEnclosure::sensors.push_back(std::move(sensor));
}

} // namespace presence
} // namespace fan
} // namespace phosphor
