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
#include "utils/pcie_card_metadata.hpp"

#include <nlohmann/json.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

class Manager;

/**
 * @class PCIeCardFloors - Action to set the PCIe card floor index parameter
 *                          based on the PCIe cards plugged in the system.
 *
 *  - Loads PCIe card metadata files using the PCIeCardMetadata class.
 *  - Watches for PCIe slots to be powered on (or off).
 *  - Reads four properties off of the PCIeDevice interface on the powered
 *    on cards.
 *  - Looks up the floor index for the card by calling PCIeCardMetadata::lookup
 *    and passing in the PCIeDevice properties.
 *  - Sets the pcie_floor_index parameter with the highest floor index found.
 *  - If no PCIe cards are found, it removes the parameter.
 *  - If a card isn't recognized, it's ignored since it isn't considered a
 *    hot card.
 *  - If a powered on card has its own temperature sensor, then it doesn't
 *    have a floor index.
 *  - Since the slot powered on indications are all sent at once, it has a
 *    small delay that gets started in each run() call that must expire
 *    before the body of the action is run, so it only runs once.
 *
 *    The JSON configuration has two entries:
 *    {
 *       "settle_time": <time in s>
 *       "use_config_specific_files": <true/false>
 *    }
 *
 *    settle_time:
 *      - Specifies how long to wait after run() is called before actually
 *        running the action as a flurry of propertiesChanged signals
 *        that trigger the action tend to come at once.
 *      - Optional.  If not specified it defaults to zero.
 *
 *    use_config_specific_files:
 *      - If true, will look for 'pcie_cards.json' files in the system
 *        specific directories.
 *      - See PCIeCardMetadata for details.
 */
class PCIeCardFloors : public ActionBase, public ActionRegister<PCIeCardFloors>
{
  public:
    /* Name of this action */
    static constexpr auto name = "pcie_card_floors";

    PCIeCardFloors() = delete;
    PCIeCardFloors(const PCIeCardFloors&) = delete;
    PCIeCardFloors(PCIeCardFloors&&) = delete;
    PCIeCardFloors& operator=(const PCIeCardFloors&) = delete;
    PCIeCardFloors& operator=(PCIeCardFloors&&) = delete;
    ~PCIeCardFloors() = default;

    /**
     * @brief Read in the action configuration
     *
     * @param[in] jsonObj - JSON configuration of this action
     * @param[in] groups - Groups of dbus objects the action uses
     */
    PCIeCardFloors(const json& jsonObj, const std::vector<Group>& groups);

    /**
     * @brief Run the action.
     *
     * Starts/restarts the settle timer and then calls execute()
     * when it expires.  Done to handle the flood of slot powered
     * on/off signals.
     *
     * @param[in] zone - Zone to run the action on
     */
    void run(Zone& zone) override;

    /**
     * @brief Set the name of the owning Event.
     *
     * In the base class it's appending it to the action's unique name.  Don't
     * do it for this action since there's only one instance of it so no need
     * to distinguish it from ones under different events and also it just
     * makes it uglier in the flight recorder.
     */
    void setEventName(const std::string& /*name*/) override
    {}

  private:
    /**
     * @brief Runs the contents of the action when the settle timer expires.
     */
    void execute();

    /**
     * @brief Constructs the PCIeCardMetadata object to load the PCIe card
     *        JSON files.
     *
     * @param[in] jsonObj - JSON configuration of this action
     */
    void loadCardJSON(const json& jsonObj);

    /**
     * @brief Returns the D-Bus object path of the card plugged into
     *        the slot represented by the slotPath D-Bus path.
     *
     * @param[in] slotPath - The D-Bus path of the PCIe slot object.
     *
     * @return const std::string& - The card object path.
     */
    const std::string& getCardFromSlot(const std::string& slotPath);

    /**
     * @brief Returns the floor index (or temp sensor name) for the
     *        card in the passed in slot.
     *
     * @param[in] slotPath - The D-Bus path of the PCIe slot object.
     *
     * @return optional<variant<int32_t, string>>
     *  - The floor index or true for has temp sensor if found,
     *    std::nullopt else.
     */
    std::optional<std::variant<int32_t, bool>>
        getFloorIndexFromSlot(const std::string& slotPath);

    /**
     * @brief Gets the hex PCIeDevice property value from the
     *        manager object cache.
     *
     * @param[in] objectPath - The card object path
     * @param[in] propertyName - The property to read
     *
     * @return uint16_t The property value
     */
    uint16_t getPCIeDeviceProperty(const std::string& objectPath,
                                   const std::string& propertyName);

    /* The PCIe card metadata manager */
    std::unique_ptr<PCIeCardMetadata> _cardMetadata;

    /* Cache map of PCIe slot paths to their plugged card paths */
    std::unordered_map<std::string, std::string> _cards;

    /* Cache of all objects with a PCIeDevice interface. */
    std::vector<std::string> _pcieDevices;

    std::chrono::seconds _settleTime{0};

    /* Timer to wait for slot plugs to settle down before running action */
    std::unique_ptr<Timer> _settleTimer;

    /* Last status printed so only new messages get recorded */
    std::string _lastStatus;
};

} // namespace phosphor::fan::control::json
