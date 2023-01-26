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
#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace phosphor::fan::control::json
{

/**
 * @class PCIeCardMetadata
 *
 * This class provides the ability for an action to look up a PCIe card's
 * fan floor index or temp sensor name based on its metadata, which
 * consists of 4 properties from the PCIeDevice D-Bus interface.
 *
 * The metadata is stored in one or more (see loadCards) JSON files, which
 * look like:
 *  {
 *    "cards": [
 *      {
 *        "name": "TestCard",
 *        "device_id": "0x1",
 *        "vendor_id": "0x2",
 *        "subsystem_id": "0x3",
 *        "subsystem_vendor_id": "0x4",
 *        "floor_index": 3
 *      },
 *      ...
 *    ]
 *  }
 *
 * If the card has a temperature sensor on it, then it doesn't
 * need a floor index and instead will have:
 *   "has_temp_sensor": true
 */
class PCIeCardMetadata
{
  public:
    PCIeCardMetadata() = delete;
    ~PCIeCardMetadata() = default;
    PCIeCardMetadata(const PCIeCardMetadata&) = delete;
    PCIeCardMetadata& operator=(const PCIeCardMetadata&) = delete;
    PCIeCardMetadata(PCIeCardMetadata&&) = delete;
    PCIeCardMetadata& operator=(PCIeCardMetadata&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] systemNames - The system names values
     */
    explicit PCIeCardMetadata(const std::vector<std::string>& systemNames);

    /**
     * @brief Look up a floor index based on a card's metadata
     *
     * @param[in] deviceID - Function0DeviceId value
     * @param[in] vendorID - Function0VendorId value
     * @param[in] subsystemID - Function0SubsystemId value
     * @param[in] subsystemVendorID - Function0SubsystemVendorId value
     *
     * @return optional<variant<int32, bool>> -
     *     Either the floor index for that entry, or true saying
     *     it has a temp sensor.
     *
     *     If no entry is found, it will return std::nullopt.
     */
    std::optional<std::variant<int32_t, bool>>
        lookup(uint16_t deviceID, uint16_t vendorID, uint16_t subsystemID,
               uint16_t subsystemVendorID) const;

  private:
    /**
     * Structure to hold card metadata.
     */
    struct Metadata
    {
        uint16_t vendorID;
        uint16_t deviceID;
        uint16_t subsystemVendorID;
        uint16_t subsystemID;
        int32_t floorIndex;
        bool hasTempSensor;

        bool operator==(const Metadata& other)
        {
            return (vendorID == other.vendorID) &&
                   (deviceID == other.deviceID) &&
                   (subsystemVendorID == other.subsystemVendorID) &&
                   (subsystemID == other.subsystemID);
        }
    };

    /**
     * @brief Loads the metadata from JSON files
     *
     * It will load a pcie_cards.json file in the default location if it
     * is present.
     *
     * If systemNames isn't empty, it will load in any 'pcie_cards.json'
     * files it finds in directories based on those names, overwriting any
     * entries that were in any previous files.  This allows different
     * floor indexes for some cards for the different systems in a flash
     * image without needing to specify every possible card in every
     * system specific JSON file.
     *
     * If no valid config files are found it will throw an exception.
     *
     * @param[in] systemNames - The system names values
     */
    void loadCards(const std::vector<std::string>& systemNames);

    /**
     * @brief Loads in the card info from the JSON
     *
     * @param[in] json - The JSON containing a cards array
     */
    void load(const nlohmann::json& json);

    /**
     * @brief Dumps the cards vector for debug
     */
    void dump() const;

    /**
     * @brief The card metadata
     */
    std::vector<Metadata> _cards;
};

} // namespace phosphor::fan::control::json
