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
#include "config.h"

#include "dbus_zone.hpp"

#include "sdbusplus.hpp"
#include "zone.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <phosphor-logging/log.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace phosphor::fan::control::json
{

using namespace phosphor::logging;
namespace fs = std::filesystem;

DBusZone::DBusZone(const Zone& zone) :
    ThermalModeIntf(util::SDBusPlus::getBus(),
                    (fs::path{CONTROL_OBJPATH} /= zone.getName()).c_str(),
                    true),
    _zone(zone)
{}

std::string DBusZone::current(std::string value)
{
    auto current = ThermalModeIntf::current();
    std::transform(value.begin(), value.end(), value.begin(), toupper);

    auto supported = ThermalModeIntf::supported();
    auto isSupported =
        std::any_of(supported.begin(), supported.end(), [&value](auto& s) {
            std::transform(s.begin(), s.end(), s.begin(), toupper);
            return value == s;
        });

    if (isSupported && value != current)
    {
        current = ThermalModeIntf::current(value);
        if (_zone.isPersisted(thermalModeIntf, currentProp))
        {
            saveCurrentMode();
        }
    }

    return current;
}

void DBusZone::restoreCurrentMode()
{
    auto current = ThermalModeIntf::current();
    fs::path path{CONTROL_PERSIST_ROOT_PATH};
    // Append this object's name and property description
    path /= _zone.getName();
    path /= "CurrentMode";
    fs::create_directories(path.parent_path());

    try
    {
        if (fs::exists(path))
        {
            std::ifstream ifs(path.c_str(), std::ios::in | std::ios::binary);
            cereal::JSONInputArchive iArch(ifs);
            iArch(current);
        }
    }
    catch (std::exception& e)
    {
        log<level::ERR>(e.what());
        fs::remove(path);
        current = ThermalModeIntf::current();
    }

    this->current(current);
}

void DBusZone::saveCurrentMode()
{
    fs::path path{CONTROL_PERSIST_ROOT_PATH};
    // Append this object's name and property description
    path /= _zone.getName();
    path /= "CurrentMode";
    std::ofstream ofs(path.c_str(), std::ios::binary);
    cereal::JSONOutputArchive oArch(ofs);
    oArch(ThermalModeIntf::current());
}

} // namespace phosphor::fan::control::json
