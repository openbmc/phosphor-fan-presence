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
#ifdef PRESENCE_JSON_FILE
#include "json_config.hpp"
#else
#include "generated.hpp"
#endif
#include "sdbusplus.hpp"
#include <sdeventplus/event.hpp>

int main(void)
{
    using namespace phosphor::fan;

    auto event = sdeventplus::Event::get_default();
    util::SDBusPlus::getBus().attach_event(
            event.get(), SD_EVENT_PRIORITY_NORMAL);

#ifdef PRESENCE_JSON_FILE
    // Use json file for presence config
    presence::JsonConfig config(PRESENCE_JSON_FILE);
    for (auto& p: config.get())
    {
        p->monitor();
    }
#else
    for (auto& p: presence::ConfigPolicy::get())
    {
        p->monitor();
    }
#endif

    return event.loop();
}
