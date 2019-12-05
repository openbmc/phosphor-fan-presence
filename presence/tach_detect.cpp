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
#if defined(PRESENCE_JSON_FILE) || \
    (defined(PRESENCE_JSON_DBUS_INTF) && defined(PRESENCE_JSON_DBUS_PROP))
#include "json_config.hpp"
#else
#include "generated.hpp"
#endif
#include "sdbusplus.hpp"
#include <sdeventplus/event.hpp>
#include <functional>
#include <stdplus/signal.hpp>
#include <sdeventplus/source/signal.hpp>

int main(void)
{
    using namespace phosphor::fan;

    auto bus = sdbusplus::bus::new_default();
    auto event = sdeventplus::Event::get_default();
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

#ifdef PRESENCE_JSON_FILE
    // Use json file for presence config
    presence::JsonConfig config(PRESENCE_JSON_FILE);
#elif defined(PRESENCE_JSON_DBUS_INTF) && defined(PRESENCE_JSON_DBUS_PROP)
    // Use first object returned in the subtree
    // (There should really only be one object with the config interface)
    std::string jsonFile = "";
    auto objects = util::SDBusPlus::getSubTree(
        bus, "/", PRESENCE_JSON_DBUS_INTF, 0);
    auto itObj = objects.begin();
    if (itObj != objects.end())
    {
        auto itServ = itObj->second.begin();
        if (itServ != itObj->second.end())
        {
            // Retrieve json config file location from dbus
            jsonFile = util::SDBusPlus::getProperty<std::string>(
                bus, itServ->first, itObj->first,
                    PRESENCE_JSON_DBUS_INTF, PRESENCE_JSON_DBUS_PROP);
        }
    }
    presence::JsonConfig config(jsonFile);
#endif

#if defined(PRESENCE_JSON_FILE) || \
    (defined(PRESENCE_JSON_DBUS_INTF) && defined(PRESENCE_JSON_DBUS_PROP))
    for (auto& p: config.get())
    {
        p->monitor();
    }

    stdplus::signal::block(SIGHUP);
    sdeventplus::source::Signal signal(event, SIGHUP,
        std::bind(&presence::JsonConfig::sighupHandler,
                  &config, std::placeholders::_1, std::placeholders::_2));
#else
    for (auto& p: presence::ConfigPolicy::get())
    {
        p->monitor();
    }
#endif

    return event.loop();
}
