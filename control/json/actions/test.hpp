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
#pragma once

#include "../zone.hpp"
#include "action.hpp"
#include "group.hpp"

#include <nlohmann/json.hpp>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

class Test : public ActionBase, public ActionRegister<Test>
{
  public:
    /* Name of this action */
    static constexpr auto name = "test";

    Test() = delete;
    Test(const Test&) = delete;
    Test(Test&&) = delete;
    Test& operator=(const Test&) = delete;
    Test& operator=(Test&&) = delete;
    ~Test() = default;

    Test(const json& jsonObj, const std::vector<Group>& groups);

    void run(Zone& zone) override;
};

} // namespace phosphor::fan::control::json

// TEST groups.json
// [
//   {
//     "name": "test",
//     "members": [
//       "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan0",
//       "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan1",
//       "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan2",
//       "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan3",
//       "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan4",
//       "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan5"
//     ]
//   },
//   {
//     "name": "test2",
//      "members": [
//        "/xyz/openbmc_project/control/thermal/0"
//      ]
//   }
// ]
// TEST groups.json
// TEST events.json
// [
//   {
//     "name": "test1",
//     "groups": [
//       { "name": "test", "interface": "xyz.openbmc_project.Inventory.Item",
//       "property": { "name": "Present" } }
//     ],
//     "triggers": [
//       { "class": "init", "method": "get_properties" }
//     ],
//     "actions": [ { "name": "test" } ]
//   },
//   {
//     "name": "test2",
//     "groups": [
//       { "name": "test2", "interface":
//       "xyz.openbmc_project.Control.ThermalMode", "property": { "name":
//       "Current" } }
//     ],
//     "triggers": [
//       { "class": "signal", "signal": "properties_changed"}
//     ],
//     "actions": [ { "name": "test" } ]
//   },
//   {
//     "name": "test3",
//     "groups": [
//       { "name": "test", "interface": "xyz.openbmc_project.Inventory.Item",
//       "property": { "name": "Present" } }
//     ],
//     "triggers": [
//       { "class": "timer", "interval": 10000000, "type": "repeating" }
//     ],
//     "actions": [ { "name": "test" } ]
//   }
// ]
// TEST events.json
//
// busctl set-property xyz.openbmc_project.Inventory.Manager
// /xyz/openbmc_project/inventory/system/chassis/motherboard/fan0
// xyz.openbmc_project.Inventory.Item Present b false
//
// busctl set-property `mapper get-service
// /xyz/openbmc_project/sensors/fan_tach`
// /xyz/openbmc_project/sensors/fan_tach/fan0_0
// xyz.openbmc_project.Control.FanSpeed Target t 5000 && busctl set-property
// `mapper get-service /xyz/openbmc_project/sensors/fan_tach`
// /xyz/openbmc_project/sensors/fan_tach/fan1_0
// xyz.openbmc_project.Control.FanSpeed Target t 5000 && busctl set-property
// `mapper get-service /xyz/openbmc_project/sensors/fan_tach`
// /xyz/openbmc_project/sensors/fan_tach/fan2_0
// xyz.openbmc_project.Control.FanSpeed Target t 5000 && busctl set-property
// `mapper get-service /xyz/openbmc_project/sensors/fan_tach`
// /xyz/openbmc_project/sensors/fan_tach/fan3_0
// xyz.openbmc_project.Control.FanSpeed Target t 5000 && busctl set-property
// `mapper get-service /xyz/openbmc_project/sensors/fan_tach`
// /xyz/openbmc_project/sensors/fan_tach/fan4_0
// xyz.openbmc_project.Control.FanSpeed Target t 5000 && busctl set-property
// `mapper get-service /xyz/openbmc_project/sensors/fan_tach`
// /xyz/openbmc_project/sensors/fan_tach/fan5_0
// xyz.openbmc_project.Control.FanSpeed Target t 5000 TEST zones.json
// [
//   {
//     "name": "0",
//     "poweron_target": 11200,
//     "default_floor": 8000,
//     "increase_delay": 5,
//     "decrease_interval": 30,
//     "interfaces": [
//       {
//         "name": "xyz.openbmc_project.Control.ThermalMode",
//         "properties": [
//           {
//             "name": "Supported",
//             "values": [
//               {
//                 "value": "DEFAULT"
//               },
//               {
//                 "value": "CUSTOM"
//               }
//             ]
//           },
//           {
//             "name": "Current",
//             "persist": true,
//             "value": "DEFAULT"
//           }
//         ]
//       }
//     ]
//   }
// ]
// TEST zones.json

// How to extract an argument by position in argument pack
// template <typename... Args>
// static decltype(auto) getBus(Args&&... args)
// {
//     // First argument is sdbusplus bus object
//     return std::get<0>(std::forward_as_tuple(args...));
// }
