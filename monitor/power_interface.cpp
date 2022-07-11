/**
 * Copyright © 2020 IBM Corporation
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
#include "power_interface.hpp"

#include "logging.hpp"
#include "sdbusplus.hpp"

namespace phosphor::fan::monitor
{

constexpr auto systemdService = "org.freedesktop.systemd1";
constexpr auto systemdPath = "/org/freedesktop/systemd1";
constexpr auto systemdMgrIface = "org.freedesktop.systemd1.Manager";

void PowerInterface::softPowerOff()
{
    util::SDBusPlus::callMethod(systemdService, systemdPath, systemdMgrIface,
                                "StartUnit", "obmc-host-shutdown@0.target",
                                "replace");
}

void PowerInterface::executeHardPowerOff()
{
    util::SDBusPlus::callMethod(
        systemdService, systemdPath, systemdMgrIface, "StartUnit",
        "obmc-chassis-hard-poweroff@0.target", "replace");

    try
    {
        util::SDBusPlus::callMethod(
            "xyz.openbmc_project.Dump.Manager", "/xyz/openbmc_project/dump/bmc",
            "xyz.openbmc_project.Dump.Create", "CreateDump",
            std::vector<
                std::pair<std::string, std::variant<std::string, uint64_t>>>());
    }
    catch (const std::exception& e)
    {
        getLogger().log(
            fmt::format("Caught exception while creating BMC dump: {}",
                        e.what()),
            Logger::error);
    }
}

void PowerInterface::hardPowerOff()
{
    executeHardPowerOff();
}

} // namespace phosphor::fan::monitor
