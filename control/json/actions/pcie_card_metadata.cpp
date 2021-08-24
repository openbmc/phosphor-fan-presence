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

#include "pcie_card_metadata.hpp"

#include "json_config.hpp"

#include <iostream> //Remove

static constexpr auto cardFileName = "pcie_cards.json";

namespace phosphor::fan::control::json
{

namespace fs = std::filesystem;
using namespace phosphor::fan;

PCIeCardMetadata::PCIeCardMetadata(const std::string& baseConfigFile,
                                   const std::vector<std::string>& systemNames)
{
    loadCards(baseConfigFile, systemNames);
    dump();
}

void PCIeCardMetadata::loadCards(const fs::path& baseConfigFile,
                                 const std::vector<std::string>& systemNames)
{
    if (!baseConfigFile.empty())
    {
        const auto basePath = fs::path{"control"} / baseConfigFile;

        // Look in the override location first
        auto confFile = fs::path{confOverridePath} / basePath;

        if (!fs::exists(confFile))
        {
            confFile = fs::path{confBasePath} / basePath;
        }

        if (!fs::exists(confFile))
        {
            throw std::runtime_error(
                fmt::format("Base PCIe card conf file {} does not exist",
                            confFile.c_str()));
        }

        fprintf(stderr, "Loading base PCIE card file %s\n", confFile.c_str());

        auto json = JsonConfig::load(confFile);

        load(json);
    }

    std::cerr << "systemNames size is " << systemNames.size() << "\n";

    for (const auto& name : systemNames)
    {
        const auto basePath = fs::path{"control"} / name / cardFileName;

        // Look in the override location first
        auto confFile = fs::path{confOverridePath} / basePath;

        if (!fs::exists(confFile))
        {
            confFile = fs::path{confBasePath} / basePath;
        }

        std::cerr << "Looking for pcie card file " << confFile.string() << "\n";
        if (fs::exists(confFile))
        {
            fprintf(stderr, "Loading PCIE card file %s\n", confFile.c_str());

            auto json = JsonConfig::load(confFile);

            load(json);
        }
    }

    if (_cards.empty())
    {
        throw std::runtime_error{
            "No valid PCIE card entries found in any JSON"};
    }

    if (!_defaultFloorIndex)
    {
        throw std::runtime_error{
            "No default floor index found in any PCIE card JSON files"};
    }

    if (!_errorFloorIndex)
    {
        throw std::runtime_error{
            "No error floor index found in any PCIE card JSON files"};
    }
}

void PCIeCardMetadata::load(const nlohmann::json& json)
{
    // Validate the cards array
    if (json.contains("cards") && json.at("cards").is_array())
    {
        for (const auto& card : json.at("cards"))
        {
            if (!card.contains("vendor_id") || !card.contains("device_id") ||
                !card.contains("subsystem_vendor_id") ||
                !card.contains("subsystem_device_id") ||
                !(card.contains("has_temp_sensor") ||
                  card.contains("floor_index")))
            {
                throw std::runtime_error{"Invalid PCIE card json"};
            }

            Metadata data;
            data.vendorID = std::stoul(card.at("vendor_id").get<std::string>(),
                                       nullptr, 16);
            data.deviceID = std::stoul(card.at("device_id").get<std::string>(),
                                       nullptr, 16);
            data.subsystemVendorID = std::stoul(
                card.at("subsystem_vendor_id").get<std::string>(), nullptr, 16);
            data.subsystemDeviceID = std::stoul(
                card.at("subsystem_device_id").get<std::string>(), nullptr, 16);

            data.hasTempSensor = card.value("has_temp_sensor", false);
            data.floorIndex = card.value("floor_index", 0);

            auto iter = std::find(_cards.begin(), _cards.end(), data);
            if (iter != _cards.end())
            {
                std::cerr << "Found existing card entry. Overwriting."
                          << "\n ";
                iter->vendorID = data.vendorID;
                iter->deviceID = data.deviceID;
                iter->subsystemVendorID = data.subsystemVendorID;
                iter->subsystemDeviceID = data.subsystemDeviceID;
                iter->floorIndex = data.floorIndex;
                iter->hasTempSensor = data.hasTempSensor;
            }
            else
            {
                _cards.push_back(std::move(data));
            }
        }
    }
    else
    {
        throw std::runtime_error{
            fmt::format("Missing 'cards' array in PCIE card JSON")};
    }

    if (json.contains("default_floor_index"))
    {
        _defaultFloorIndex = json.at("default_floor_index").get<uint32_t>();
    }

    if (json.contains("error_floor_index"))
    {
        _errorFloorIndex = json.at("error_floor_index").get<uint32_t>();
    }
}

void PCIeCardMetadata::dump()
{
    std::cerr << "default floor index: " << _defaultFloorIndex << "\n";
    std::cerr << "error floor index: " << _errorFloorIndex << "\n";

    for (const auto& entry : _cards)
    {
        std::cerr << "--------------------------------------------------"
                  << "\n";
        std::cerr << "vendorID: " << std::hex << entry.vendorID << "\n";
        std::cerr << "deviceID: " << std::hex << entry.deviceID << "\n";
        std::cerr << "subsysVendorID: " << std::hex << entry.subsystemVendorID
                  << "\n";
        std::cerr << "subsystemDeviceID: " << std::hex
                  << entry.subsystemDeviceID << "\n";
        std::cerr << "hasTempSensor: " << std::hex << entry.hasTempSensor
                  << "\n";
        std::cerr << "floorIndex: " << std::hex << entry.floorIndex << "\n";
    }
}

std::tuple<bool, int32_t>
    PCIeCardMetadata::lookup(const std::string& vendorID,
                             const std::string& deviceID,
                             const std::string& subsystemVendorID,
                             const std::string& subsystemDeviceID) const
{
    return {false, 0};
}

} // namespace phosphor::fan::control::json
