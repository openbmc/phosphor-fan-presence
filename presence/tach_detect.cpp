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
#include <vector>
#include <sdbusplus/bus.hpp>
#include "fan_enclosure.hpp"
#include "fan_detect_defs.hpp"
#include "tach_sensor.hpp"


int main(void)
{
    auto bus = sdbusplus::bus::new_default();

    std::vector<std::unique_ptr<phosphor::fan::presence::FanEnclosure>> fans;

    for (auto const& detectMap: fanDetectMap)
    {
        if (detectMap.first == "tach")
        {
            for (auto const& fanProp: detectMap.second)
            {
                auto fan = std::make_unique<
                    phosphor::fan::presence::FanEnclosure>(bus,
                                                           fanProp);
                for (auto const &fanSensor: std::get<2>(fanProp))
                {
                    auto sensor = std::make_unique<
                        phosphor::fan::presence::TachSensor>(bus,
                                                             fanSensor,
                                                             *fan);
                    fan->addSensor(std::move(sensor));
                }
                fans.push_back(std::move(fan));
            }
        }
    }

    while (true)
    {
        // Respond to dbus signals
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
