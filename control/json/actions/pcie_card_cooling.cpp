/**
 * Copyright © 2021 IBM Corporation
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

#include "pcie_card_cooling.hpp"

#include "json_config.hpp"

namespace phosphor::fan::control::json
{

// "/xyz/openbmc_project/inventory/system/chassis/motherboard/pcieslot9",
// "/xyz/openbmc_project/inventory/system/chassis/motherboard/pcieslot9/pcie_cardXX",
// "xyz.openbmc_project.State.Decorator.PowerState",

PCIeCardCooling::PCIeCardCooling(const json& jsonObj,
                                 const std::vector<Group>& groups) :
    ActionBase(jsonObj, groups)
{
    loadCardJSON(jsonObj);
}

void PCIeCardCooling::run(Zone& zone)
{
    std::cerr << ">>PCIeCardCooling::run()"
              << "\n";
    _manager = zone.getManager();

    // calcFloorIndex();
}

void PCIeCardCooling::loadCardJSON(const json& jsonObj)
{
    std::string baseConfigFile;
    bool useConfigSpecificFiles = false;

    if (jsonObj.contains("base_pcie_card_file"))
    {
        baseConfigFile = jsonObj.at("base_pcie_card_file").get<std::string>();
    }

    if (jsonObj.contains("use_config_specific_files"))
    {
        useConfigSpecificFiles =
            jsonObj.at("use_config_specific_files").get<bool>();
    }

    if (baseConfigFile.empty() && !useConfigSpecificFiles)
    {
        throw ActionParseError{
            ActionBase::getName(),
            "Missing base_config_file or use_config_specific_files entries"};
    }

    std::vector<std::string> names;
    if (useConfigSpecificFiles)
    {
        names = phosphor::fan::JsonConfig::getCompatValues();
    }

    _cardMetadata = std::make_unique<PCIeCardMetadata>(baseConfigFile, names);
}

// void PCIeCardCooling::calcFloorIndex()
// {
//     try
//     {
//         auto floorValues = getFloorTableIndices();
//         if (!floorValues.empty())
//         {
//             std::sort(floorValues.begin(), floorValues.end());
//             updatePropertyCache(floorValues.back());
//         }
//     }
//     catch (const std::exception& e)
//     {
//         log<level::INFO>(e.what());
//         // update to backup floor value from the metadata?
//     }
// }

// std::vector<int32_t> PCIeCardCooling::getFloorTableIndices()
// {

//     // Need to deal with power state interface and pcie device interface!
// }

void PCIeCardCooling::updatePropertyCache(int32_t floorIndex)
{
    assert(_manager);
    const std::string name{"PCIE_FLOOR_INDEX}";
    _manager->setProperty(name, name, name, floorIndex);
}

} // namespace phosphor::fan::control::json
