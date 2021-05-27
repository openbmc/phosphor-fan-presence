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

#ifndef CONTROL_USE_JSON
#include "argument.hpp"
#include "manager.hpp"
#else
#include "json/manager.hpp"
#endif
#include "sdbusplus.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>
#include <stdplus/signal.hpp>

using namespace phosphor::fan::control;
using namespace phosphor::logging;

int main(int argc, char* argv[])
{
    auto event = sdeventplus::Event::get_default();

#ifndef CONTROL_USE_JSON
    phosphor::fan::util::ArgumentParser args(argc, argv);
    if (argc != 2)
    {
        args.usage(argv);
        return 1;
    }

    Mode mode;

    if (args["init"] == "true")
    {
        mode = Mode::init;
    }
    else if (args["control"] == "true")
    {
        mode = Mode::control;
    }
    else
    {
        args.usage(argv);
        return 1;
    }
#endif

    // Attach the event object to the bus object so we can
    // handle both sd_events (for the timers) and dbus signals.
    phosphor::fan::util::SDBusPlus::getBus().attach_event(
        event.get(), SD_EVENT_PRIORITY_NORMAL);

    try
    {
#ifdef CONTROL_USE_JSON
        json::Manager manager(event);

        // Enable SIGHUP handling to reload JSON configs
        stdplus::signal::block(SIGHUP);
        sdeventplus::source::Signal signal(
            event, SIGHUP,
            std::bind(&json::Manager::sighupHandler, &manager,
                      std::placeholders::_1, std::placeholders::_2));

        phosphor::fan::util::SDBusPlus::getBus().request_name(CONTROL_BUSNAME);
#else
        Manager manager(phosphor::fan::util::SDBusPlus::getBus(), event, mode);

        // Init mode will just set fans to max and delay
        if (mode == Mode::init)
        {
            manager.doInit(event);
            return 0;
        }
#endif
        return event.loop();
    }
    // Log the useful metadata on these exceptions and let the app
    // return 1 so it is restarted without a core dump.
    catch (phosphor::fan::util::DBusServiceError& e)
    {
        log<level::ERR>("Uncaught DBus service lookup failure exception",
                        entry("PATH=%s", e.path.c_str()),
                        entry("INTERFACE=%s", e.interface.c_str()));
    }
    catch (phosphor::fan::util::DBusMethodError& e)
    {
        log<level::ERR>("Uncaught DBus method failure exception",
                        entry("BUSNAME=%s", e.busName.c_str()),
                        entry("PATH=%s", e.path.c_str()),
                        entry("INTERFACE=%s", e.interface.c_str()),
                        entry("METHOD=%s", e.method.c_str()));
    }
    catch (phosphor::fan::util::DBusPropertyError& e)
    {
        log<level::ERR>("Uncaught DBus property access failure exception",
                        entry("BUSNAME=%s", e.busName.c_str()),
                        entry("PATH=%s", e.path.c_str()),
                        entry("INTERFACE=%s", e.interface.c_str()),
                        entry("PROPERTY=%s", e.property.c_str()));
    }

    return 1;
}
