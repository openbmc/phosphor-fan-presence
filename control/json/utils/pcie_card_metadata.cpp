/**
 * Copyright © 2022 IBM Corporation
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
#include "utils/flight_recorder.hpp"

#include <fmt/format.h>

#include <iostream>

static constexpr auto cardFileName = "pcie_cards.json";

namespace phosphor::fan::control::json
{

namespace fs = std::filesystem;
using namespace phosphor::fan;

PCIeCardMetadata::PCIeCardMetadata(const std::vector<std::string>& systemNames)
{
    loadCards(systemNames);
}

void PCIeCardMetadata::loadCards(const std::vector<std::string>& systemNames)
{
    const auto defaultPath = fs::path{"control"} / cardFileName;

    // Look in the override location first
    auto confFile = fs::path{confOverridePath} / defaultPath;

    if (!fs::exists(confFile))
    {
        confFile = fs::path{confBasePath} / defaultPath;
    }

    if (fs::exists(confFile))
    {
        FlightRecorder::instance().log(
            "main",
            fmt::format("Loading configuration from {}", confFile.string()));
        load(JsonConfig::load(confFile));
        FlightRecorder::instance().log(
            "main", fmt::format("Configuration({}) loaded successfully",
                                confFile.string()));
        log<level::INFO>(fmt::format("Configuration({}) loaded successfully",
                                     confFile.string())
                             .c_str());
    }

    // Go from least specific to most specific in the system names so files in
    // the latter category can override ones in the former.
    for (auto nameIt = systemNames.rbegin(); nameIt != systemNames.rend();
         ++nameIt)
    {
        const auto basePath = fs::path{"control"} / *nameIt / cardFileName;

        // Look in the override location first
        auto confFile = fs::path{confOverridePath} / basePath;

        if (!fs::exists(confFile))
        {
            confFile = fs::path{confBasePath} / basePath;
        }

        if (fs::exists(confFile))
        {
            FlightRecorder::instance().log(
                "main", fmt::format("Loading configuration from {}",
                                    confFile.string()));
            load(JsonConfig::load(confFile));
            FlightRecorder::instance().log(
                "main", fmt::format("Configuration({}) loaded successfully",
                                    confFile.string()));
            log<level::INFO>(
                fmt::format("Configuration({}) loaded successfully",
                            confFile.string())
                    .c_str());
        }
    }

    if (_cards.empty())
    {
        throw std::runtime_error{
            "No valid PCIe card entries found in any JSON"};
    }
}

void PCIeCardMetadata::load(const nlohmann::json& json)
{
    if (!json.contains("cards") || !json.at("cards").is_array())
    {
        throw std::runtime_error{
            fmt::format("Missing 'cards' array in PCIe card JSON")};
    }

    for (const auto& card : json.at("cards"))
    {
        if (!card.contains("vendor_id") || !card.contains("device_id") ||
            !card.contains("subsystem_vendor_id") ||
            !card.contains("subsystem_id") ||
            !(card.contains("has_temp_sensor") || card.contains("floor_index")))
        {
            throw std::runtime_error{"Invalid PCIe card json"};
        }

        Metadata data;
        data.vendorID =
            std::stoul(card.at("vendor_id").get<std::string>(), nullptr, 16);
        data.deviceID =
            std::stoul(card.at("device_id").get<std::string>(), nullptr, 16);
        data.subsystemVendorID = std::stoul(
            card.at("subsystem_vendor_id").get<std::string>(), nullptr, 16);
        data.subsystemID =
            std::stoul(card.at("subsystem_id").get<std::string>(), nullptr, 16);

        data.hasTempSensor = card.value("has_temp_sensor", false);
        data.floorIndex = card.value("floor_index", -1);

        auto iter = std::find(_cards.begin(), _cards.end(), data);
        if (iter != _cards.end())
        {
            iter->vendorID = data.vendorID;
            iter->deviceID = data.deviceID;
            iter->subsystemVendorID = data.subsystemVendorID;
            iter->subsystemID = data.subsystemID;
            iter->floorIndex = data.floorIndex;
            iter->hasTempSensor = data.hasTempSensor;
        }
        else
        {
            _cards.push_back(std::move(data));
        }
    }
}

void PCIeCardMetadata::dump() const
{
    for (const auto& entry : _cards)
    {
        std::cerr << "--------------------------------------------------"
                  << "\n";
        std::cerr << "vendorID: " << std::hex << entry.vendorID << "\n";
        std::cerr << "deviceID: " << entry.deviceID << "\n";
        std::cerr << "subsysVendorID: " << entry.subsystemVendorID << "\n";
        std::cerr << "subsystemID: " << entry.subsystemID << "\n";
        std::cerr << "hasTempSensor: " << std::dec << entry.hasTempSensor
                  << "\n";
        std::cerr << "floorIndex: " << entry.floorIndex << "\n";
    }
}

std::optional<std::variant<int32_t, bool>>
    PCIeCardMetadata::lookup(uint16_t deviceID, uint16_t vendorID,
                             uint16_t subsystemID,
                             uint16_t subsystemVendorID) const
{
    log<level::DEBUG>(fmt::format("Lookup {:#x} ${:#x} {:#x} {:#x}", deviceID,
                                  vendorID, subsystemID, subsystemVendorID)
                          .c_str());
    auto card =
        std::find_if(_cards.begin(), _cards.end(),
                     [&deviceID, &vendorID, &subsystemID,
                      &subsystemVendorID](const auto& card) {
                         return (deviceID == card.deviceID) &&
                                (vendorID == card.vendorID) &&
                                (subsystemID == card.subsystemID) &&
                                (subsystemVendorID == card.subsystemVendorID);
                     });

    if (card != _cards.end())
    {
        if (card->hasTempSensor)
        {
            return true;
        }
        return card->floorIndex;
    }
    return std::nullopt;
}

} // namespace phosphor::fan::control::json
