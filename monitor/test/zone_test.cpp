// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "../multichassis_json_parser.hpp"
#include "../multichassis_types.hpp"
#include "../tach_sensor.hpp"
#include "../zone.hpp"

#include <gtest/gtest.h>

using namespace phosphor::fan::monitor::multi_chassis;
using json = nlohmann::json;

TEST(ZoneTest, PowerOnTest)
{
    const auto zoneConfig = R"(
        {
            "zones": [
                {
                    "name": "${chassis}",
                    "fans": [
                        {
                            "type": "systemFan",
                            "inventory_base": "/xyz/openbmc_project/inventory/system/chassis${chassis}/motherboard/",
                            "short_name": "chassis${chassis}_fan0"
                        },
                        {
                            "type": "systemFan",
                            "inventory_base": "/xyz/openbmc_project/inventory/system/chassis${chassis}/motherboard/",
                            "short_name": "chassis${chassis}_fan1"
                        },
                        {
                            "type": "systemFan",
                            "inventory_base": "/xyz/openbmc_project/inventory/system/chassis${chassis}/motherboard/",
                            "short_name": "chassis${chassis}_fan2"
                        },
                        {
                            "type": "systemFan",
                            "inventory_base": "/xyz/openbmc_project/inventory/system/chassis${chassis}/motherboard/",
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
        })"_json;
    std::vector<int> chassisNums = {0};
    std::vector<ZoneDefinition> zoneDefs =
        getZoneDefs(zoneConfig["zones"], chassisNums);

    const auto fanTypes = R"(
        {
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
        ]
        })"_json;

    std::vector<FanTypeDefinition> fanTypeList =
        getFanDefs(fanTypes["fan_type_definitions"]);
    auto event = sdeventplus::Event::get_default();
    auto bus = sdbusplus::bus::new_default();
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);
    phosphor::fan::monitor::ThermalAlertObject thermalAlert(
        bus, THERMAL_ALERT_OBJPATH);
    phosphor::fan::PowerState powerState;
    for (const auto& zoneDef : zoneDefs)
    {
        Zone zone(zoneDef, fanTypeList, bus,
                  phosphor::fan::monitor::Mode::monitor, event, thermalAlert,
                  powerState);
        EXPECT_FALSE(zone.isPowerOn()); // Will be false, since can't power on
                                        // from unit test
    }
}

TEST(ZoneTest, FanCreationTest)
{
    const auto zoneConfig = R"(
    {
        "zones": [
            {
                "name": "${chassis}",
                "fans": [
                    {
                        "type": "systemFan",
                        "inventory_base": "/xyz/openbmc_project/inventory/system/chassis${chassis}/motherboard/",
                        "short_name": "chassis${chassis}_fan0"
                    },
                    {
                        "type": "systemFan",
                        "inventory_base": "/xyz/openbmc_project/inventory/system/chassis${chassis}/motherboard/",
                        "short_name": "chassis${chassis}_fan1"
                    },
                    {
                        "type": "systemFan",
                        "inventory_base": "/xyz/openbmc_project/inventory/system/chassis${chassis}/motherboard/",
                        "short_name": "chassis${chassis}_fan2"
                    },
                    {
                        "type": "systemFan",
                        "inventory_base": "/xyz/openbmc_project/inventory/system/chassis${chassis}/motherboard/",
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
    })"_json;
    std::vector<int> chassisNums = {0};
    std::vector<ZoneDefinition> zoneDefs =
        getZoneDefs(zoneConfig["zones"], chassisNums);

    const auto fanTypes = R"(
        {
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
        ]
        })"_json;

    std::vector<FanTypeDefinition> fanTypeList =
        getFanDefs(fanTypes["fan_type_definitions"]);
    auto event = sdeventplus::Event::get_default();
    auto bus = sdbusplus::bus::new_default();
    bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);
    phosphor::fan::monitor::ThermalAlertObject thermalAlert(
        bus, THERMAL_ALERT_OBJPATH);
    phosphor::fan::PowerState powerState;
    for (const auto& zoneDef : zoneDefs)
    {
        Zone zone(zoneDef, fanTypeList, bus,
                  phosphor::fan::monitor::Mode::monitor, event, thermalAlert,
                  powerState);
        zone.init(zoneDef, fanTypeList);
        EXPECT_EQ(zone.getFans().size(), 4); // Each zone should have 4 fans
    }
}
