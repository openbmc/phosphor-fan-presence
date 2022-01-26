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
#include "config.h"
#ifdef PRESENCE_USE_JSON
#include "json_config.hpp"
#include "json_parser.hpp"
#include "utility.hpp"
#else
#include "generated.hpp"
#endif
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>
#include <stdplus/signal.hpp>

#include <functional>

void loadJsonAndStart(sdbusplus::bus::bus& bus, sdeventplus::Event& event);

namespace presence = phosphor::fan::presence;

int main(void)
{
    using namespace phosphor::fan;

    auto bus = sdbusplus::bus::new_default();
    auto event = sdeventplus::Event::get_default();
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

#ifdef PRESENCE_USE_JSON

    namespace match = sdbusplus::bus::match;
    using Match = match::match;

    auto waitForInventory = std::make_unique<Match>(
        bus, match::rules::nameOwnerChanged(util::INVENTORY_SVC),
        [&bus, &event](auto& msg) {
            std::string msgStr;
            msg.read(msgStr);

            // first string is inteface, make sure it's for us
            if (util::INVENTORY_INTF != msgStr)
            {
                return;
            }

            msg.read(msgStr); // old name, not used
            msg.read(msgStr); // new name

            if (!msgStr.empty())
            {
                loadJsonAndStart(bus, event);
            }
        });

    bool invServiceRunning = util::SDBusPlus::callMethodAndRead<bool>(
        bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
        "org.freedesktop.DBus", "NameHasOwner", util::INVENTORY_SVC);

    if (invServiceRunning)
    {
        waitForInventory.reset();
        loadJsonAndStart(bus, event);
    }

#else
    for (auto& p : presence::ConfigPolicy::get())
    {
        p->monitor();
    }
#endif

    return event.loop();
}

#ifdef PRESENCE_USE_JSON
void loadJsonAndStart(sdbusplus::bus::bus& bus, sdeventplus::Event& event)
{
    presence::JsonConfig config(bus);

    // jsonConfig will call config::start when
    // the conf file is available.
    phosphor::fan::JsonConfig jsonConfig{
        std::bind(&presence::JsonConfig::start, &config)};

    stdplus::signal::block(SIGHUP);
    sdeventplus::source::Signal signal(
        event, SIGHUP,
        std::bind(&presence::JsonConfig::sighupHandler, &config,
                  std::placeholders::_1, std::placeholders::_2));
}
#endif
