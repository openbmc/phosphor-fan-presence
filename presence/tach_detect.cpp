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
#include <vector>
#include "fan_enclosure.hpp"
#include "fan_detect_defs.hpp"
#include "sdbusplus.hpp"
#include "tach_sensor.hpp"


int main(void)
{
    using namespace phosphor::fan;
    using namespace std::literals::string_literals;
    using namespace phosphor::logging;

    std::vector<std::unique_ptr<phosphor::fan::presence::FanEnclosure>> fans;

    for (auto const& detectMap: fanDetectMap)
    {
        if (detectMap.first == "tach")
        {
            for (auto const& fanProp: detectMap.second)
            {
                auto path = "/xyz/openbmc_project/inventory"s +
                    std::get<0>(fanProp);

                auto state = presence::UNKNOWN;
                try
                {
                    auto boolstate = util::SDBusPlus::getProperty<bool>(
                            path,
                            "xyz.openbmc_project.Inventory.Item"s,
                            "Present"s);
                    state = boolstate ?
                        presence::PRESENT : presence::NOT_PRESENT;
                }
                catch (const std::exception& err)
                {
                    log<level::INFO>(err.what());
                }

                auto fan = std::make_unique<
                    phosphor::fan::presence::FanEnclosure>(fanProp,
                                                           state);
                for (auto const &fanSensor: std::get<2>(fanProp))
                {
                    auto initialSpeed = static_cast<int64_t>(0);
                    auto sensorPath =
                        "/xyz/openbmc_project/sensors/fan_tach/"s +
                        fanSensor;
                    initialSpeed = util::SDBusPlus::getProperty<int64_t>(
                            sensorPath,
                            "xyz.openbmc_project.Sensor.Value"s,
                            "Value"s);

                    auto sensor = std::make_unique<
                        phosphor::fan::presence::TachSensor>(fanSensor,
                                                             *fan,
                                                             initialSpeed != 0);
                    fan->addSensor(std::move(sensor));
                }

                fan->updInventory();
                fans.push_back(std::move(fan));
            }
        }
    }

    while (true)
    {
        // Respond to dbus signals
        util::SDBusPlus::getBus().process_discard();
        util::SDBusPlus::getBus().wait();
    }
    return 0;
}
