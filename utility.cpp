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
#include <string>
#include "utility.hpp"

namespace phosphor
{
namespace fan
{
namespace util
{

using namespace std::string_literals;

//TODO Should get these from phosphor-objmgr config.h
constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";

//TODO Should get these from phosphor-inventory-manager config.h
const auto INVENTORY_PATH = "/xyz/openbmc_project/inventory"s;
const auto INVENTORY_INTF = "xyz.openbmc_project.Inventory.Manager"s;

std::string getInvService(sdbusplus::bus::bus& bus)
{
    return getService(INVENTORY_PATH, INVENTORY_INTF, bus);
}


std::string getService(const std::string& path,
                       const std::string& interface,
                       sdbusplus::bus::bus& bus)
{
    auto mapperCall = bus.new_method_call(MAPPER_BUSNAME,
                                          MAPPER_PATH,
                                          MAPPER_INTERFACE,
                                          "GetObject");

    mapperCall.append(path);
    mapperCall.append(std::vector<std::string>({interface}));

    auto mapperResponseMsg = bus.call(mapperCall);
    if (mapperResponseMsg.is_method_error())
    {
        throw std::runtime_error(
            "Error in mapper call to get service name");
    }


    std::map<std::string, std::vector<std::string>> mapperResponse;
    mapperResponseMsg.read(mapperResponse);

    if (mapperResponse.empty())
    {
        throw std::runtime_error(
            "Error in mapper response for getting service name");
    }

    return mapperResponse.begin()->first;
}

}
}
}
