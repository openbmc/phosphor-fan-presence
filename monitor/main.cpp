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
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include "fan.hpp"
#include "fan_defs.hpp"

using namespace phosphor::fan::monitor;
using namespace phosphor::logging;


void EventDeleter(sd_event* event)
{
    sd_event_unref(event);
}

int main()
{
    auto bus = sdbusplus::bus::new_default();
    sd_event* events = nullptr;
    std::vector<std::unique_ptr<Fan>> fans;

    auto r = sd_event_default(&events);
    if (r < 0)
    {
        log<level::ERR>("Failed call to sd_event_default()",
                        entry("ERROR=%s", strerror(-r)));
        return -1;
    }

    std::shared_ptr<sd_event> eventPtr{events, EventDeleter};

    //Attach the event object to the bus object so we can
    //handle both sd_events (for the timers) and dbus signals.
    bus.attach_event(eventPtr.get(), SD_EVENT_PRIORITY_NORMAL);

    for (const auto& fanDef : fanDefinitions)
    {
        fans.emplace_back(std::make_unique<Fan>(bus, eventPtr, fanDef));
    }

    r = sd_event_loop(eventPtr.get());
    if (r < 0)
    {
        log<level::ERR>("Failed call to sd_event_loop",
                        entry("ERROR=%s", strerror(-r)));
    }

    return -1;
}
