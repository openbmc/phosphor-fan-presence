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
#include "power_state.hpp"
#include "shutdown_alarm_monitor.hpp"
#include "threshold_alarm_logger.hpp"

#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

using namespace sensor::monitor;

int main(int, char*[])
{
    auto event = sdeventplus::Event::get_default();
    auto bus = sdbusplus::bus::new_default();
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

#ifdef ENABLE_HOST_STATE
    std::shared_ptr<phosphor::fan::PowerState> powerState =
        std::make_shared<phosphor::fan::HostPowerState>();
#else
    std::shared_ptr<phosphor::fan::PowerState> powerState =
        std::make_shared<phosphor::fan::PGoodState>();
#endif

    ShutdownAlarmMonitor shutdownMonitor{bus, event, powerState};

    ThresholdAlarmLogger logger{bus, event, powerState};

    return event.loop();
}
