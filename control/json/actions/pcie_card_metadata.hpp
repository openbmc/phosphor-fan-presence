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
#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>
#include <tuple>
#include <vector>

namespace phosphor::fan::control::json
{

/**
 * @class PCIeCardMetadata
 *
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

    PCIeCardMetadata(const std::string& baseConfigFile,
                     const std::vector<std::string>& systemNames);

    std::tuple<bool, int32_t>
        lookup(const std::string& vendorID, const std::string& deviceID,
               const std::string& subsystemVendorID,
               const std::string& subsystemDeviceID) const;

  private:
    struct Metadata
    {
        uint16_t vendorID;
        uint16_t deviceID;
        uint16_t subsystemVendorID;
        uint16_t subsystemDeviceID;
        bool hasTempSensor;
        uint32_t floorIndex;

        bool operator==(const Metadata& other)
        {
            return (vendorID == other.vendorID) &&
                   (deviceID == other.deviceID) &&
                   (subsystemVendorID == other.subsystemVendorID) &&
                   (subsystemDeviceID == other.subsystemDeviceID);
        }
    };

    void loadCards(const std::filesystem::path& baseConfigFile,
                   const std::vector<std::string>& systemNames);
    void load(const nlohmann::json& json);

    void dump();

    std::vector<Metadata> _cards;

    uint32_t _defaultFloorIndex{0};
    uint32_t _errorFloorIndex{0};
};

} // namespace phosphor::fan::control::json
