/**
 * Copyright © 2017 IBM Corporation
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
#include <map>
#include <sdbusplus/message.hpp>
#include <string>
#include "fan.hpp"
#include "sdbusplus.hpp"

namespace phosphor
{
namespace fan
{
namespace presence
{

using namespace std::literals::string_literals;

static const auto invNamespace = "/xyz/openbmc_project/inventory"s;
static const auto itemIface = "xyz.openbmc_project.Inventory.Item"s;
static const auto invMgrIface = "xyz.openbmc_project.Inventory.Manager"s;

void setPresence(const Fan& fan, bool newState)
{
    using namespace sdbusplus::message;

    using Properties = std::map<std::string, variant<std::string, bool>>;
    using Interfaces = std::map<std::string, Properties>;

    std::map<object_path, Interfaces> obj =
    {{
        std::get<1>(fan),
        {{
            itemIface,
            {
                {"Present"s, newState},
                {"PrettyName"s, std::get<0>(fan)},
            }
        }}
    }};

    util::SDBusPlus::lookupAndCallMethod(
            invNamespace,
            invMgrIface,
            "Notify"s,
            obj);
}

bool getPresence(const Fan& fan)
{
    return util::SDBusPlus::getProperty<bool>(
            std::get<1>(fan),
            itemIface,
            "Present"s);
}

} // namespace presence
} // namespace fan
} // namespace phosphor
