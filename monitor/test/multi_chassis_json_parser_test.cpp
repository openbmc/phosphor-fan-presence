// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "../multichassis_json_parser.hpp"
#include "../multichassis_types.hpp"

#include <gtest/gtest.h>

using namespace phosphor::fan::monitor::multi_chassis;
using json = nlohmann::json;

TEST(MultiChassisJsonParserTest, SensorDefsTest)
{
    const auto sensorConfig = R"(
    {
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
    })"_json;
    std::vector<SensorDefinition> actual =
        getSensorDefs(sensorConfig["sensors"]);

    std::vector<SensorDefinition> expected = {
        SensorDefinition{
            .pathSuffix = "_0",
            .hasTarget = true,
            .targetInterface = "xyz.openbmc_project.Control.FanSpeed",
            .targetPath = "",
            .factor = 1.0,
            .offset = 0,
            .threshold = 30,
            .ignoreAboveMax = false},
        SensorDefinition{
            .pathSuffix = "_1",
            .hasTarget = false,
            .targetInterface = "xyz.openbmc_project.Control.FanSpeed",
            .targetPath = "",
            .factor = 0.625,
            .offset = 1100,
            .threshold = 30,
            .ignoreAboveMax = false}};

    ASSERT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < actual.size(); i++)
    {
        EXPECT_EQ(actual[i].pathSuffix, expected[i].pathSuffix);
        EXPECT_EQ(actual[i].hasTarget, expected[i].hasTarget);
        EXPECT_EQ(actual[i].targetInterface, expected[i].targetInterface);
        EXPECT_EQ(actual[i].targetPath, expected[i].targetPath);
        EXPECT_DOUBLE_EQ(actual[i].factor, expected[i].factor);
        EXPECT_EQ(actual[i].offset, expected[i].offset);
        EXPECT_EQ(actual[i].threshold, expected[i].threshold);
        EXPECT_EQ(actual[i].ignoreAboveMax, expected[i].ignoreAboveMax);
    }
}

TEST(MultiChassisJsonParserTest, FanDefsTest)
{
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
    std::vector<FanTypeDefinition> actual =
        getFanDefs(fanTypes["fan_type_definitions"]);

    std::vector<FanTypeDefinition> expected = {FanTypeDefinition{
        .type = "systemFan",
        .method = phosphor::fan::monitor::MethodMode::count,
        .funcDelay = 0,
        .timeout = 0,
        .deviation = 18,
        .upperDeviation = 18,
        .numSensorFailsForNonfunc = 0,
        .monitorStartDelay = 30,
        .countInterval = 1,
        .nonfuncRotorErrDelay = 0,
        .fanMissingErrDelay = 20,
        .sensorList =
            {SensorDefinition{
                 .pathSuffix = "_0",
                 .hasTarget = true,
                 .targetInterface = "xyz.openbmc_project.Control.FanSpeed",
                 .targetPath = "",
                 .factor = 1.0,
                 .offset = 0,
                 .threshold = 30,
                 .ignoreAboveMax = false},
             SensorDefinition{
                 .pathSuffix = "_1",
                 .hasTarget = false,
                 .targetInterface = "xyz.openbmc_project.Control.FanSpeed",
                 .targetPath = "",
                 .factor = 0.625,
                 .offset = 1100,
                 .threshold = 30,
                 .ignoreAboveMax = false}},
        .condition = std::optional<phosphor::fan::monitor::Condition>(),
        .funcOnPresent = true}};

    ASSERT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < actual.size(); i++)
    {
        EXPECT_EQ(actual[i].type, expected[i].type);
        EXPECT_EQ(actual[i].method, expected[i].method);
        EXPECT_EQ(actual[i].funcDelay, expected[i].funcDelay);
        EXPECT_EQ(actual[i].timeout, expected[i].timeout);
        EXPECT_EQ(actual[i].deviation, expected[i].deviation);
        EXPECT_EQ(actual[i].upperDeviation, expected[i].upperDeviation);
        EXPECT_EQ(actual[i].numSensorFailsForNonfunc,
                  expected[i].numSensorFailsForNonfunc);
        EXPECT_EQ(actual[i].monitorStartDelay, expected[i].monitorStartDelay);
        EXPECT_EQ(actual[i].countInterval, expected[i].countInterval);
        EXPECT_EQ(actual[i].nonfuncRotorErrDelay,
                  expected[i].nonfuncRotorErrDelay);
        EXPECT_EQ(actual[i].fanMissingErrDelay, expected[i].fanMissingErrDelay);
        ASSERT_EQ(actual[i].sensorList.size(), expected[i].sensorList.size());
        for (size_t j = 0; j < actual[i].sensorList.size(); j++)
        {
            EXPECT_EQ(actual[i].sensorList[j].pathSuffix,
                      expected[i].sensorList[j].pathSuffix);
            EXPECT_EQ(actual[i].sensorList[j].hasTarget,
                      expected[i].sensorList[j].hasTarget);
            EXPECT_EQ(actual[i].sensorList[j].targetInterface,
                      expected[i].sensorList[j].targetInterface);
            EXPECT_EQ(actual[i].sensorList[j].targetPath,
                      expected[i].sensorList[j].targetPath);
            EXPECT_DOUBLE_EQ(actual[i].sensorList[j].factor,
                             expected[i].sensorList[j].factor);
            EXPECT_EQ(actual[i].sensorList[j].offset,
                      expected[i].sensorList[j].offset);
            EXPECT_EQ(actual[i].sensorList[j].threshold,
                      expected[i].sensorList[j].threshold);
            EXPECT_EQ(actual[i].sensorList[j].ignoreAboveMax,
                      expected[i].sensorList[j].ignoreAboveMax);
        }
        EXPECT_EQ(actual[i].condition.has_value(),
                  expected[i].condition.has_value());
        EXPECT_EQ(actual[i].funcOnPresent, expected[i].funcOnPresent);
    }
}

TEST(MultiChassisJsonParserTest, FanAssignsTest)
{
    const auto fanAssigns = R"(
    {
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
        ]
    })"_json;
    std::vector<FanAssignment> actual = getFanAssigns(fanAssigns["fans"], 0);

    std::vector<FanAssignment> expected = {
        FanAssignment{
            .type = "systemFan",
            .inventoryBase =
                "/xyz/openbmc_project/inventory/system/chassis0/motherboard/",
            .shortName = "chassis0_fan0"},
        FanAssignment{
            .type = "systemFan",
            .inventoryBase =
                "/xyz/openbmc_project/inventory/system/chassis0/motherboard/",
            .shortName = "chassis0_fan1"},
        FanAssignment{
            .type = "systemFan",
            .inventoryBase =
                "/xyz/openbmc_project/inventory/system/chassis0/motherboard/",
            .shortName = "chassis0_fan2"},
        FanAssignment{
            .type = "systemFan",
            .inventoryBase =
                "/xyz/openbmc_project/inventory/system/chassis0/motherboard/",
            .shortName = "chassis0_fan3"}};

    ASSERT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < actual.size(); i++)
    {
        EXPECT_EQ(actual[i].type, expected[i].type);
        EXPECT_EQ(actual[i].inventoryBase, expected[i].inventoryBase);
        EXPECT_EQ(actual[i].shortName, expected[i].shortName);
    }
}

TEST(MultiChassisJsonParserTest, ZoneDefsTest)
{
    const auto zoneConfig = R"(
    {
        "zones": [
            {
                "name": "chassis_${chassis}_zone",
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
    std::vector<ZoneDefinition> actual = getZoneDefs(zoneConfig["zones"], 0);

    const auto faultHandlingConfig = R"(
    {
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
    })"_json;
    std::vector<ZoneDefinition> expected = {ZoneDefinition{
        .name = "chassis_0_zone",
        .fans =
            {FanAssignment{
                 .type = "systemFan",
                 .inventoryBase =
                     "/xyz/openbmc_project/inventory/system/chassis0/motherboard/",
                 .shortName = "chassis0_fan0"},
             FanAssignment{
                 .type = "systemFan",
                 .inventoryBase =
                     "/xyz/openbmc_project/inventory/system/chassis0/motherboard/",
                 .shortName = "chassis0_fan1"},
             FanAssignment{
                 .type = "systemFan",
                 .inventoryBase =
                     "/xyz/openbmc_project/inventory/system/chassis0/motherboard/",
                 .shortName = "chassis0_fan2"},
             FanAssignment{
                 .type = "systemFan",
                 .inventoryBase =
                     "/xyz/openbmc_project/inventory/system/chassis0/motherboard/",
                 .shortName = "chassis0_fan3"}},
        .faultHandling = faultHandlingConfig}};

    ASSERT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < actual.size(); i++)
    {
        EXPECT_EQ(actual[i].name, expected[i].name);
        ASSERT_EQ(actual[i].fans.size(), expected[i].fans.size());
        for (size_t j = 0; j < actual[i].fans.size(); j++)
        {
            EXPECT_EQ(actual[i].fans[j].type, expected[i].fans[j].type);
            EXPECT_EQ(actual[i].fans[j].inventoryBase,
                      expected[i].fans[j].inventoryBase);
            EXPECT_EQ(actual[i].fans[j].shortName,
                      expected[i].fans[j].shortName);
        }
        EXPECT_EQ(actual[i].faultHandling, expected[i].faultHandling);
    }
}

TEST(MultiChassisJsonParserTest, ChassisDefsTest)
{
    const auto chassisConfig = R"({
        "chassis_definitions": [
            {
                "name": "chassis${chassis}",
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
                        "name": "chassis_${chassis}_zone",
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
            }
        ]
    })"_json;
    std::vector<ChassisDefinition> actual =
        getChassisDefs(chassisConfig["chassis_definitions"]);

    const auto faultHandlingConfig = R"(
    {
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
    })"_json;
    std::vector<ChassisDefinition> expected = {
        ChassisDefinition{
            .name = "chassis0",
            .chassisNumbers = {0, 1, 2, 3, 4, 5, 6, 7},
            .zones = {ZoneDefinition{
                .name = "chassis_0_zone",
                .fans =
                    {FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis0/motherboard/",
                         .shortName = "chassis0_fan0"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis0/motherboard/",
                         .shortName = "chassis0_fan1"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis0/motherboard/",
                         .shortName = "chassis0_fan2"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis0/motherboard/",
                         .shortName = "chassis0_fan3"}},
                .faultHandling = faultHandlingConfig}},
            .availPropUsed = true,
            .chassisNumRef = 0},
        ChassisDefinition{
            .name = "chassis1",
            .chassisNumbers = {0, 1, 2, 3, 4, 5, 6, 7},
            .zones = {ZoneDefinition{
                .name = "chassis_1_zone",
                .fans =
                    {FanAssignment{
                         .type =
                             "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis1/motherboard/",
                         .shortName = "chassis1_fan0"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis1/motherboard/",
                         .shortName = "chassis1_fan1"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis1/motherboard/",
                         .shortName = "chassis1_fan2"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis1/motherboard/",
                         .shortName = "chassis1_fan3"}},
                .faultHandling = faultHandlingConfig}},
            .availPropUsed = true,
            .chassisNumRef = 1},
        ChassisDefinition{
            .name = "chassis2",
            .chassisNumbers = {0, 1, 2, 3, 4, 5, 6, 7},
            .zones = {ZoneDefinition{
                .name = "chassis_2_zone",
                .fans =
                    {FanAssignment{
                         .type =
                             "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis2/motherboard/",
                         .shortName = "chassis2_fan0"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis2/motherboard/",
                         .shortName = "chassis2_fan1"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis2/motherboard/",
                         .shortName = "chassis2_fan2"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis2/motherboard/",
                         .shortName = "chassis2_fan3"}},
                .faultHandling = faultHandlingConfig}},
            .availPropUsed = true,
            .chassisNumRef = 2},
        ChassisDefinition{
            .name = "chassis3",
            .chassisNumbers = {0, 1, 2, 3, 4, 5, 6, 7},
            .zones = {ZoneDefinition{
                .name = "chassis_3_zone",
                .fans =
                    {FanAssignment{
                         .type =
                             "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis3/motherboard/",
                         .shortName = "chassis3_fan0"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis3/motherboard/",
                         .shortName = "chassis3_fan1"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis3/motherboard/",
                         .shortName = "chassis3_fan2"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis3/motherboard/",
                         .shortName = "chassis3_fan3"}},
                .faultHandling = faultHandlingConfig}},
            .availPropUsed = true,
            .chassisNumRef = 3},
        ChassisDefinition{
            .name = "chassis4",
            .chassisNumbers = {0, 1, 2, 3, 4, 5, 6, 7},
            .zones = {ZoneDefinition{
                .name = "chassis_4_zone",
                .fans =
                    {FanAssignment{
                         .type =
                             "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis4/motherboard/",
                         .shortName = "chassis4_fan0"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis4/motherboard/",
                         .shortName = "chassis4_fan1"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis4/motherboard/",
                         .shortName = "chassis4_fan2"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis4/motherboard/",
                         .shortName = "chassis4_fan3"}},
                .faultHandling = faultHandlingConfig}},
            .availPropUsed = true,
            .chassisNumRef = 4},
        ChassisDefinition{
            .name = "chassis5",
            .chassisNumbers = {0, 1, 2, 3, 4, 5, 6, 7},
            .zones = {ZoneDefinition{
                .name = "chassis_5_zone",
                .fans =
                    {FanAssignment{
                         .type =
                             "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis5/motherboard/",
                         .shortName = "chassis5_fan0"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis5/motherboard/",
                         .shortName = "chassis5_fan1"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis5/motherboard/",
                         .shortName = "chassis5_fan2"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis5/motherboard/",
                         .shortName = "chassis5_fan3"}},
                .faultHandling = faultHandlingConfig}},
            .availPropUsed = true,
            .chassisNumRef = 5},
        ChassisDefinition{
            .name = "chassis6",
            .chassisNumbers = {0, 1, 2, 3, 4, 5, 6, 7},
            .zones = {ZoneDefinition{
                .name = "chassis_6_zone",
                .fans =
                    {FanAssignment{
                         .type =
                             "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis6/motherboard/",
                         .shortName = "chassis6_fan0"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis6/motherboard/",
                         .shortName = "chassis6_fan1"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis6/motherboard/",
                         .shortName = "chassis6_fan2"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis6/motherboard/",
                         .shortName = "chassis6_fan3"}},
                .faultHandling = faultHandlingConfig}},
            .availPropUsed = true,
            .chassisNumRef = 6},
        ChassisDefinition{
            .name = "chassis7",
            .chassisNumbers = {0, 1, 2, 3, 4, 5, 6, 7},
            .zones = {ZoneDefinition{
                .name = "chassis_7_zone",
                .fans =
                    {FanAssignment{
                         .type =
                             "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis7/motherboard/",
                         .shortName = "chassis7_fan0"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis7/motherboard/",
                         .shortName = "chassis7_fan1"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis7/motherboard/",
                         .shortName = "chassis7_fan2"},
                     FanAssignment{
                         .type = "systemFan",
                         .inventoryBase =
                             "/xyz/openbmc_project/inventory/system/chassis7/motherboard/",
                         .shortName = "chassis7_fan3"}},
                .faultHandling = faultHandlingConfig}},
            .availPropUsed = true,
            .chassisNumRef = 7}};

    ASSERT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < actual.size(); i++)
    {
        EXPECT_EQ(actual[i].chassisNumbers, expected[i].chassisNumbers);
        ASSERT_EQ(actual[i].zones.size(), expected[i].zones.size());
        for (size_t j = 0; j < actual[i].zones.size(); j++)
        {
            EXPECT_EQ(actual[i].zones[j].name, expected[i].zones[j].name);
            ASSERT_EQ(actual[i].zones[j].fans.size(),
                      expected[i].zones[j].fans.size());
            for (size_t k = 0; k < actual[i].zones[j].fans.size(); k++)
            {
                EXPECT_EQ(actual[i].zones[j].fans[k].type,
                          expected[i].zones[j].fans[k].type);
                EXPECT_EQ(actual[i].zones[j].fans[k].inventoryBase,
                          expected[i].zones[j].fans[k].inventoryBase);
                EXPECT_EQ(actual[i].zones[j].fans[k].shortName,
                          expected[i].zones[j].fans[k].shortName);
            }
            EXPECT_EQ(actual[i].zones[j].faultHandling,
                      expected[i].zones[j].faultHandling);
        }
        EXPECT_EQ(actual[i].availPropUsed, expected[i].availPropUsed);
        EXPECT_EQ(actual[i].chassisNumRef, expected[i].chassisNumRef);
    }
}

TEST(MultiChassisJsonParserTest, TypeErrorTest)
{
    // Zone config has a name that is a number here, rather than a string.
    // Test checks that a type error is thrown when attempting to parse this.
    const auto zoneConfig = R"(
    {
        "zones": [
            {
                "name": 0,
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
    })"_json;

    EXPECT_THROW(getZoneDefs(zoneConfig["zones"], 0), json::type_error);
}

TEST(MultiChassisJsonParserTest, RuntimeErrorTest)
{
    // Attempts to parse a fan_type_definitions object with no 'type' field.
    // This should throw an std::runtime_error.
    const auto fanTypes = R"(
    {
        "fan_type_definitions": [
        {
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

    EXPECT_THROW(getFanDefs(fanTypes["fan_type_definitions"]),
                 std::runtime_error);
}
