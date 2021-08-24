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

#include "../zone.hpp"
#include "action.hpp"
#include "group.hpp"

#include <nlohmann/json.hpp>

#include "pcie_card_metadata.hpp"

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

class Manager;

/**
 * @class PCIeCardCooling -
 */

class PCIeCardCooling :
    public ActionBase,
    public ActionRegister<PCIeCardCooling>
{
  public:
    /* Name of this action */
    static constexpr auto name = "pcie_card_cooling";

    PCIeCardCooling() = delete;
    PCIeCardCooling(const PCIeCardCooling&) = delete;
    PCIeCardCooling(PCIeCardCooling&&) = delete;
    PCIeCardCooling& operator=(const PCIeCardCooling&) = delete;
    PCIeCardCooling& operator=(PCIeCardCooling&&) = delete;
    ~PCIeCardCooling() = default;

    /**
     * @brief Parse the JSON to set the members
     *
     * @param[in] jsonObj - JSON configuration of this action
     * @param[in] groups - Groups of dbus objects the action uses
     */
    PCIeCardCooling(const json& jsonObj, const std::vector<Group>& groups);

    /**
     * @brief Run the action.  See description above.
     *
     * @param[in] zone - Zone to run the action on
     */
    void run(Zone& zone) override;

  private:

    void loadCardJSON(const json& jsonObj);
    void updatePropertyCache(int32_t floorIndex);
    std::vector<int32_t> getFloorTableIndices();

    std::unique_ptr<PCIeCardMetadata> _cardMetadata;

    Manager* _manager{nullptr};

    // sdbusplus::bus::match::match _deviceAddedMatch;

    // sdbusplus::bus::match::match _deviceRemovedMatch;
};

} // namespace phosphor::fan::control::json
