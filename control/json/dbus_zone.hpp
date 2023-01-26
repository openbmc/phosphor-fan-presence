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

#include "xyz/openbmc_project/Control/ThermalMode/server.hpp"

/* Extend the Control::ThermalMode interface */
using ThermalModeIntf = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::Control::server::ThermalMode>;

namespace phosphor::fan::control::json
{

class Zone;

class DBusZone : public ThermalModeIntf
{
  public:
    static constexpr auto thermalModeIntf =
        "xyz.openbmc_project.Control.ThermalMode";
    static constexpr auto supportedProp = "Supported";
    static constexpr auto currentProp = "Current";

    DBusZone() = delete;
    DBusZone(const DBusZone&) = delete;
    DBusZone(DBusZone&&) = delete;
    DBusZone& operator=(const DBusZone&) = delete;
    DBusZone& operator=(DBusZone&&) = delete;
    ~DBusZone() = default;

    /**
     * Constructor
     * Creates a thermal control dbus object associated with the given zone
     *
     * @param[in] zone - Zone object
     */
    explicit DBusZone(const Zone& zone);

    /**
     * @brief Overridden thermalmode interface's set 'Current' property function
     *
     * @param[in] value - Value to set 'Current' to
     *
     * @return - The updated value of the 'Current' property
     */
    std::string current(std::string value) override;

    /**
     * @brief Restore persisted thermalmode `Current` mode property value,
     * setting the mode to "Default" otherwise
     */
    void restoreCurrentMode();

  private:
    /* Zone object associated with this thermal control dbus object */
    const Zone& _zone;

    /**
     * @brief Save the thermalmode `Current` mode property to persisted storage
     */
    void saveCurrentMode();
};

} // namespace phosphor::fan::control::json
