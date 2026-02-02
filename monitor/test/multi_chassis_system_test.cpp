// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "../chassis.hpp"
#include "../multichassis_json_parser.hpp"
#include "../multichassis_system.hpp"
#include "../multichassis_types.hpp"

#include <gtest/gtest.h>

using namespace phosphor::fan::monitor::multi_chassis;
using json = nlohmann::json;

class MultiChassisSystemTest : public testing::Test
{
  protected:
    MultiChassisSystemTest() :
        bus(sdbusplus::bus::new_default()),
        event(sdeventplus::Event::get_default())
    {
        jsonObj = R"({
            "fan_type_definitions": [
                {
                    "type": "systemFan",
                    "method": "count",
                    "count_interval": 1,
                    "deviation": 18,
                    "num_sensors_nonfunc_for_fan_nonfunc": 0,
                    "monitor_start_delay": 30,
                    "fan_missing_error_delay": 20,
                    "nonfunc_rotor_error_delay": 0,
                    "set_func_on_present": true,
                    "sensors": [
                        {
                            "path_suffix": "_0",
                            "threshold": 30,
                            "has_target": true
                        },
                        {
                            "path_suffix": "_1",
                            "threshold": 30,
                            "has_target": false,
                            "factor": 0.625,
                            "offset": 1100
                        }
                    ]
                }
            ],
            "chassis_definitions": [
                {
                    "availPropUsed": true,
                    "chassis_numbers": [
                        0,
                        1,
                        2,
                        3,
                        4,
                        5,
                        6,
                        7
                    ],
                    "zones": [
                        {
                            "name": "${chassis}",
                            "fans": [
                                {
                                    "type": "systemFan",
                                    "inventory_base": "/xyz/openbmc_project/inventory/system/${chassis}/motherboard/",
                                    "short_name": "chassis${chassis}_fan0"
                                },
                                {
                                    "type": "systemFan",
                                    "inventory_base": "/xyz/openbmc_project/inventory/system/${chassis}/motherboard/",
                                    "short_name": "chassis${chassis}_fan1"
                                },
                                {
                                    "type": "systemFan",
                                    "inventory_base": "/xyz/openbmc_project/inventory/system/${chassis}/motherboard/",
                                    "short_name": "chassis${chassis}_fan2"
                                },
                                {
                                    "type": "systemFan",
                                    "inventory_base": "/xyz/openbmc_project/inventory/system/${chassis}/motherboard/",
                                    "short_name": "chassis${chassis}_fan3"
                                }
                            ],
                            "fault_handling": {
                                "power_off_config": [
                                    {
                                        "type": "epow",
                                        "cause": "fan_frus_with_nonfunc_rotors",
                                        "count": 2,
                                        "service_mode_delay": 300,
                                        "meltdown_delay": 300
                                    }
                                ]
                            }
                        }
                    ]
                }
            ]
        })"_json;
        bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);
    }
    json jsonObj;
    sdbusplus::bus_t bus;
    sdeventplus::Event event;
};

TEST_F(MultiChassisSystemTest, MultiChassisSystemCreationTest)
{
    // Create System object and call initChassis()
    // Maybe expose internal chassis objects for testing?
    std::vector<FanTypeDefinition> fanTypeDefs =
        getFanDefs(jsonObj["fan_type_definitions"]);
    std::vector<ChassisDefinition> chassisDefs =
        getChassisDefs(jsonObj["chassis_definitions"]);
    MultiChassisSystem system(phosphor::fan::monitor::Mode::monitor, bus,
                              event);
    system.initChassis(chassisDefs, fanTypeDefs);
    auto& chassisList = system.getChassis();
    std::vector<int> chassisNums = {0, 1, 2, 3, 4, 5, 6, 7};
    for (int chassisNum : chassisNums)
    {
        auto& chassis = chassisList.at(chassisNum);
        std::string chassisName = std::to_string(chassisNum);
        // init chassis
        chassis->init();
        // check properties (available, availPropUsed, chassisName)
        EXPECT_FALSE(chassis->isAvailable());
        EXPECT_TRUE(chassis->isAvailPropUsed());
        EXPECT_EQ(chassis->getName(), chassisName);
    }
}
