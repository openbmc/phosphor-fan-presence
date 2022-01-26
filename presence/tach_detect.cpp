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

namespace presence = phosphor::fan::presence;

int main(void)
{
    auto event = sdeventplus::Event::get_default();

#ifdef PRESENCE_USE_JSON
    auto bus = sdbusplus::bus::new_default();
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);
    presence::JsonConfig jsonConfig(bus, event);
#else
    for (auto& p : presence::ConfigPolicy::get())
    {
        p->monitor();
    }
#endif

    return event.loop();
}
