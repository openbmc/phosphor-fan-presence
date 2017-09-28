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
#include <systemd/sd-daemon.h>
#include "argument.hpp"
#include "event.hpp"
#include "fan.hpp"
#include "fan_defs.hpp"
#include "trust_manager.hpp"

using namespace phosphor::fan::monitor;
using namespace phosphor::logging;

int main(int argc, char* argv[])
{
    auto bus = sdbusplus::bus::new_default();
    sd_event* events = nullptr;
    std::vector<std::unique_ptr<Fan>> fans;
    phosphor::fan::util::ArgumentParser args(argc, argv);

    if (argc != 2)
    {
        args.usage(argv);
        exit(-1);
    }

    Mode mode;
    if (args["init"] == "true")
    {
        mode = Mode::init;
    }
    else if (args["monitor"] == "true")
    {
        mode = Mode::monitor;
    }
    else
    {
        args.usage(argv);
        exit(-1);
    }

    auto r = sd_event_default(&events);
    if (r < 0)
    {
        log<level::ERR>("Failed call to sd_event_default()",
                        entry("ERROR=%s", strerror(-r)));
        return -1;
    }

    std::unique_ptr<phosphor::fan::trust::Manager> trust =
            std::make_unique<phosphor::fan::trust::Manager>(trustGroups);

    phosphor::fan::event::EventPtr eventPtr{events};

    //Attach the event object to the bus object so we can
    //handle both sd_events (for the timers) and dbus signals.
    bus.attach_event(eventPtr.get(), SD_EVENT_PRIORITY_NORMAL);

    for (const auto& fanDef : fanDefinitions)
    {
        fans.emplace_back(std::make_unique<Fan>(
                mode, bus, eventPtr, trust, fanDef));
    }

    if (mode == Mode::init)
    {
        // Fans were initialized to be functional, exit
        return 0;
    }
    else
    {
        r = sd_event_loop(eventPtr.get());
        if (r < 0)
        {
            log<level::ERR>("Failed call to sd_event_loop",
                            entry("ERROR=%s", strerror(-r)));
        }
    }

    return -1;
}
