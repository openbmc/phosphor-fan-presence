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
#include "pcie_card_floors.hpp"

#include "../manager.hpp"
#include "json_config.hpp"
#include "sdbusplus.hpp"
#include "sdeventplus.hpp"

namespace phosphor::fan::control::json
{

constexpr auto floorIndexParam = "pcie_floor_index";
constexpr auto pcieDeviceIface =
    "xyz.openbmc_project.Inventory.Item.PCIeDevice";
constexpr auto powerStateIface =
    "xyz.openbmc_project.State.Decorator.PowerState";
constexpr auto deviceIDProp = "Function0DeviceId";
constexpr auto vendorIDProp = "Function0VendorId";
constexpr auto subsystemIDProp = "Function0SubsystemId";
constexpr auto subsystemVendorIDProp = "Function0SubsystemVendorId";

PCIeCardFloors::PCIeCardFloors(const json& jsonObj,
                               const std::vector<Group>& groups) :
    ActionBase(jsonObj, groups)
{
    loadCardJSON(jsonObj);
}

void PCIeCardFloors::run(Zone& zone)
{
    if (_settleTimer)
    {
        _settleTimer->setEnabled(false);
    }
    else
    {
        _settleTimer =
            std::make_unique<Timer>(util::SDEventPlus::getEvent(),
                                    [&zone, this](Timer&) { execute(); });
    }
    _settleTimer->restartOnce(_settleTime);
}

void PCIeCardFloors::execute()
{
    size_t hotCards = 0;
    size_t numTempSensorCards = 0;
    size_t uninterestingCards = 0;
    int32_t floorIndex = -1;

    for (const auto& group : _groups)
    {
        if (group.getInterface() != powerStateIface)
        {
            log<level::DEBUG>(
                fmt::format("Wrong interface {} in PCIe card floor group",
                            group.getInterface())
                    .c_str());
            continue;
        }

        for (const auto& slotPath : group.getMembers())
        {
            PropertyVariantType powerState;

            try
            {
                powerState = Manager::getObjValueVariant(
                    slotPath, group.getInterface(), group.getProperty());
            }
            catch (const std::out_of_range& oore)
            {
                log<level::ERR>(
                    fmt::format("Could not get power state for {}", slotPath)
                        .c_str());
                continue;
            }

            if (std::get<std::string>(powerState) !=
                "xyz.openbmc_project.State.Decorator.PowerState.State.On")
            {
                continue;
            }

            auto floorIndexOrTempSensor = getFloorIndexFromSlot(slotPath);
            if (floorIndexOrTempSensor)
            {
                if (std::holds_alternative<int32_t>(*floorIndexOrTempSensor))
                {
                    hotCards++;
                    floorIndex = std::max(
                        floorIndex, std::get<int32_t>(*floorIndexOrTempSensor));
                }
                else
                {
                    numTempSensorCards++;
                }
            }
            else
            {
                uninterestingCards++;
            }
        }
    }

    auto status = fmt::format(
        "Found {} hot cards, {} with temp sensors, {} uninteresting", hotCards,
        numTempSensorCards, uninterestingCards);
    if (status != _lastStatus)
    {
        record(status);
        _lastStatus = status;
    }

    int32_t origIndex = -1;
    auto origIndexVariant = Manager::getParameter(floorIndexParam);
    if (origIndexVariant)
    {
        origIndex = std::get<int32_t>(*origIndexVariant);
    }

    if (floorIndex != -1)
    {
        if (origIndex != floorIndex)
        {
            record(fmt::format("Setting {} parameter to {}", floorIndexParam,
                               floorIndex));
            Manager::setParameter(floorIndexParam, floorIndex);
        }
    }
    else if (origIndexVariant)
    {
        record(fmt::format("Removing parameter {}", floorIndexParam));
        Manager::setParameter(floorIndexParam, std::nullopt);
    }
}

void PCIeCardFloors::loadCardJSON(const json& jsonObj)
{
    bool useConfigSpecificFiles = false;

    if (jsonObj.contains("settle_time"))
    {
        _settleTime =
            std::chrono::seconds(jsonObj.at("settle_time").get<size_t>());
    }

    if (jsonObj.contains("use_config_specific_files"))
    {
        useConfigSpecificFiles =
            jsonObj.at("use_config_specific_files").get<bool>();
    }

    std::vector<std::string> names;
    if (useConfigSpecificFiles)
    {
        names = phosphor::fan::JsonConfig::getCompatValues();
    }

    _cardMetadata = std::make_unique<PCIeCardMetadata>(names);
}

uint16_t PCIeCardFloors::getPCIeDeviceProperty(const std::string& objectPath,
                                               const std::string& propertyName)
{
    PropertyVariantType variantValue;
    uint16_t value{};

    try
    {
        variantValue = Manager::getObjValueVariant(objectPath, pcieDeviceIface,
                                                   propertyName);
    }
    catch (const std::out_of_range& oore)
    {
        log<level::ERR>(
            fmt::format(
                "{}: Could not get PCIeDevice property {} {} from cache ",
                ActionBase::getName(), objectPath, propertyName)
                .c_str());
        throw;
    }

    try
    {
        value = std::stoul(std::get<std::string>(variantValue), nullptr, 0);
        return value;
    }
    catch (const std::invalid_argument& e)
    {
        log<level::INFO>(
            fmt::format("{}: {} has invalid PCIeDevice property {} value: {}",
                        ActionBase::getName(), objectPath, propertyName,
                        std::get<std::string>(variantValue))
                .c_str());
        throw;
    }
}

std::optional<std::variant<int32_t, bool>>
    PCIeCardFloors::getFloorIndexFromSlot(const std::string& slotPath)
{
    const auto& card = getCardFromSlot(slotPath);

    try
    {
        auto deviceID = getPCIeDeviceProperty(card, deviceIDProp);
        auto vendorID = getPCIeDeviceProperty(card, vendorIDProp);
        auto subsystemID = getPCIeDeviceProperty(card, subsystemIDProp);
        auto subsystemVendorID =
            getPCIeDeviceProperty(card, subsystemVendorIDProp);

        return _cardMetadata->lookup(deviceID, vendorID, subsystemID,
                                     subsystemVendorID);
    }
    catch (const std::exception& e)
    {}

    return std::nullopt;
}

const std::string& PCIeCardFloors::getCardFromSlot(const std::string& slotPath)
{
    auto cardIt = _cards.find(slotPath);

    if (cardIt != _cards.end())
    {
        return cardIt->second;
    }

    // Just the first time, find all the PCIeDevice objects
    if (_pcieDevices.empty())
    {
        _pcieDevices = util::SDBusPlus::getSubTreePaths(
            util::SDBusPlus::getBus(), "/", pcieDeviceIface, 0);
    }

    // Find the card that plugs in this slot based on if the
    // slot is part of the path, like slotA/cardA
    auto it = std::find_if(
        _pcieDevices.begin(), _pcieDevices.end(), [slotPath](const auto& path) {
            return path.find(slotPath + '/') != std::string::npos;
        });

    if (it == _pcieDevices.end())
    {
        throw std::runtime_error(fmt::format(
            "Could not find PCIe card object path for slot {}", slotPath));
    }

    _cards.emplace(slotPath, *it);

    return _cards.at(slotPath);
}

} // namespace phosphor::fan::control::json
