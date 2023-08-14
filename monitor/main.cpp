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

#ifndef MONITOR_USE_JSON
#include <CLI/CLI.hpp>
#endif
#include "fan.hpp"
#ifdef MONITOR_USE_JSON
#include "dbus_paths.hpp"
#include "json_config.hpp"
#include "json_parser.hpp"
#endif
#include "system.hpp"
#include "trust_manager.hpp"

#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>
#include <stdplus/signal.hpp>

using namespace phosphor::fan::monitor;

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    auto event = sdeventplus::Event::get_default();
    auto bus = sdbusplus::bus::new_default();
    Mode mode = Mode::init;

#ifndef MONITOR_USE_JSON
    CLI::App app{"Phosphor Fan Monitor"};

    bool init = false;
    bool monitor = false;
    app.add_flag("-i,--init", init, "Set fans to functional");
    app.add_flag("-m,--monitor", monitor, "Start fan functional monitoring");
    app.require_option();

    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::Error& e)
    {
        return app.exit(e);
    }

    if (init)
    {
        mode = Mode::init;
    }
    else if (monitor)
    {
        mode = Mode::monitor;
    }
#endif

    // Attach the event object to the bus object so we can
    // handle both sd_events (for the timers) and dbus signals.
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

    System system(mode, bus, event);

#ifdef MONITOR_USE_JSON

    phosphor::fan::JsonConfig config(std::bind(&System::start, &system));

    // Enable SIGHUP handling to reload JSON config
    stdplus::signal::block(SIGHUP);
    sdeventplus::source::Signal signal(event, SIGHUP,
                                       std::bind(&System::sighupHandler,
                                                 &system, std::placeholders::_1,
                                                 std::placeholders::_2));

    // Enable SIGUSR1 handling to dump debug data
    stdplus::signal::block(SIGUSR1);
    sdeventplus::source::Signal sigUsr1(
        event, SIGUSR1,
        std::bind(&System::dumpDebugData, &system, std::placeholders::_1,
                  std::placeholders::_2));

    bus.request_name(THERMAL_ALERT_BUSNAME);
#else
    system.start();

    if (mode == Mode::init)
    {
        // Fans were initialized to be functional, exit
        return 0;
    }
#endif

    return event.loop();
}
