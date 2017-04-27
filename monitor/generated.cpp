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
#include "fan_defs.hpp"
#include "types.hpp"

//This file will be replaced by a generated version in a later commit

using namespace phosphor::fan::monitor;

const std::vector<FanDefinition> fanDefinitions
{

    FanDefinition{"/system/chassis/motherboard/fan0",
                    30,
                    15,
                    .85,
                    1,
                    std::vector<SensorDefinition>{
                        SensorDefinition{"fan0", true}
                    }},
    FanDefinition{"/system/chassis/motherboard/fan1",
                    30,
                    15,
                    .85,
                    1,
                    std::vector<SensorDefinition>{
                        SensorDefinition{"fan1", true}
                    }},
    FanDefinition{"/system/chassis/motherboard/fan2",
                    30,
                    15,
                    .85,
                    1,
                    std::vector<SensorDefinition>{
                        SensorDefinition{"fan2", true}
                    }},
    FanDefinition{"/system/chassis/motherboard/fan3",
                    30,
                    15,
                    .85,
                    1,
                    std::vector<SensorDefinition>{
                        SensorDefinition{"fan3", true}
                    }}
};
