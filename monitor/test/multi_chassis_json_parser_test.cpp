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
                "inventory": "/system/chassis${chassis}/motherboard/fan0",
                "short_name": "chassis${chassis}_fan0",
                "pretty_name": "Node ${node} fan 0",
                "presence_gpio": "fan0_chassis${chassis}_presence",
                "presence_gpio_polarity": "Low",
                "presence_tach_redundancy": false
            },
            {
                "type": "systemFan",
                "inventory": "/system/chassis${chassis}/motherboard/fan1",
                "short_name": "chassis${chassis}_fan1",
                "pretty_name": "Node ${node} fan 1",
                "presence_gpio": "fan1_chassis${chassis}_presence",
                "presence_gpio_polarity": "High",
                "presence_tach_redundancy": true
            },
            {
                "type": "systemFan",
                "inventory": "/system/chassis${chassis}/motherboard/fan2",
                "short_name": "chassis${chassis}_fan2",
                "pretty_name": "Node ${node} fan 2",
                "presence_gpio": "fan2_chassis${chassis}_presence",
                "presence_gpio_polarity": "Low"
            },
            {
                "type": "systemFan",
                "inventory": "/system/chassis${chassis}/motherboard/fan3",
                "short_name": "chassis${chassis}_fan3",
                "pretty_name": "Node ${node} fan 3",
                "presence_gpio": "fan3_chassis${chassis}_presence",
                "presence_gpio_polarity": "High"
            }
        ]
    })"_json;
    std::vector<FanAssignment> actual = getFanAssigns(fanAssigns["fans"], 1);

    std::vector<FanAssignment> expected = {
        FanAssignment{.type = "systemFan",
                      .inventory = "/system/chassis1/motherboard/fan0",
                      .shortName = "chassis1_fan0",
                      .gpioInfo = GpioInfo{.name = "fan0_chassis1_presence",
                                           .isActiveLow = true,
                                           .tachRedundancy = false,
                                           .fanPrettyName = "Node 0 fan 0"}},
        FanAssignment{.type = "systemFan",
                      .inventory = "/system/chassis1/motherboard/fan1",
                      .shortName = "chassis1_fan1",
                      .gpioInfo = GpioInfo{.name = "fan1_chassis1_presence",
                                           .isActiveLow = false,
                                           .tachRedundancy = true,
                                           .fanPrettyName = "Node 0 fan 1"}},
        FanAssignment{.type = "systemFan",
                      .inventory = "/system/chassis1/motherboard/fan2",
                      .shortName = "chassis1_fan2",
                      .gpioInfo = GpioInfo{.name = "fan2_chassis1_presence",
                                           .isActiveLow = true,
                                           .tachRedundancy = true,
                                           .fanPrettyName = "Node 0 fan 2"}},
        FanAssignment{.type = "systemFan",
                      .inventory = "/system/chassis1/motherboard/fan3",
                      .shortName = "chassis1_fan3",
                      .gpioInfo = GpioInfo{.name = "fan3_chassis1_presence",
                                           .isActiveLow = false,
                                           .tachRedundancy = true,
                                           .fanPrettyName = "Node 0 fan 3"}}};

    ASSERT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < actual.size(); i++)
    {
        EXPECT_EQ(actual[i].type, expected[i].type);
        EXPECT_EQ(actual[i].inventory, expected[i].inventory);
        EXPECT_EQ(actual[i].shortName, expected[i].shortName);
        EXPECT_EQ(actual[i].gpioInfo.name, expected[i].gpioInfo.name);
        EXPECT_EQ(actual[i].gpioInfo.isActiveLow,
                  expected[i].gpioInfo.isActiveLow);
        EXPECT_EQ(actual[i].gpioInfo.tachRedundancy,
                  expected[i].gpioInfo.tachRedundancy);
        EXPECT_EQ(actual[i].gpioInfo.fanPrettyName,
                  expected[i].gpioInfo.fanPrettyName);
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
                        "inventory": "/system/chassis${chassis}/motherboard/fan0",
                        "short_name": "chassis${chassis}_fan0",
                        "pretty_name": "Node ${node} fan 0",
                        "presence_gpio": "fan0_chassis${chassis}_presence",
                        "presence_gpio_polarity": "Low"
                    },
                    {
                        "type": "systemFan",
                        "inventory": "/system/chassis${chassis}/motherboard/fan1",
                        "short_name": "chassis${chassis}_fan1",
                        "pretty_name": "Node ${node} fan 1",
                        "presence_gpio": "fan1_chassis${chassis}_presence",
                        "presence_gpio_polarity": "Low"
                    },
                    {
                        "type": "systemFan",
                        "inventory": "/system/chassis${chassis}/motherboard/fan2",
                        "short_name": "chassis${chassis}_fan2",
                        "pretty_name": "Node ${node} fan 2",
                        "presence_gpio": "fan2_chassis${chassis}_presence",
                        "presence_gpio_polarity": "Low"
                    },
                    {
                        "type": "systemFan",
                        "inventory": "/system/chassis${chassis}/motherboard/fan3",
                        "short_name": "chassis${chassis}_fan3",
                        "pretty_name": "Node ${node} fan 3",
                        "presence_gpio": "fan3_chassis${chassis}_presence",
                        "presence_gpio_polarity": "Low"
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
    std::vector<ZoneDefinition> actual = getZoneDefs(zoneConfig["zones"], 1);

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
        .name = "chassis_1_zone",
        .fans = {FanAssignment{
                     .type = "systemFan",
                     .inventory = "/system/chassis1/motherboard/fan0",
                     .shortName = "chassis1_fan0",
                     .gpioInfo = GpioInfo{.name = "fan0_chassis1_presence",
                                          .isActiveLow = true,
                                          .tachRedundancy = true,
                                          .fanPrettyName = "Node 0 fan 0"}},
                 FanAssignment{
                     .type = "systemFan",
                     .inventory = "/system/chassis1/motherboard/fan1",
                     .shortName = "chassis1_fan1",
                     .gpioInfo = GpioInfo{.name = "fan1_chassis1_presence",
                                          .isActiveLow = true,
                                          .tachRedundancy = true,
                                          .fanPrettyName = "Node 0 fan 1"}},
                 FanAssignment{
                     .type = "systemFan",
                     .inventory = "/system/chassis1/motherboard/fan2",
                     .shortName = "chassis1_fan2",
                     .gpioInfo = GpioInfo{.name = "fan2_chassis1_presence",
                                          .isActiveLow = true,
                                          .tachRedundancy = true,
                                          .fanPrettyName = "Node 0 fan 2"}},
                 FanAssignment{
                     .type = "systemFan",
                     .inventory = "/system/chassis1/motherboard/fan3",
                     .shortName = "chassis1_fan3",
                     .gpioInfo = GpioInfo{.name = "fan3_chassis1_presence",
                                          .isActiveLow = true,
                                          .tachRedundancy = true,
                                          .fanPrettyName = "Node 0 fan 3"}}},
        .faultHandling = faultHandlingConfig}};

    ASSERT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < actual.size(); i++)
    {
        EXPECT_EQ(actual[i].name, expected[i].name);
        ASSERT_EQ(actual[i].fans.size(), expected[i].fans.size());
        for (size_t j = 0; j < actual[i].fans.size(); j++)
        {
            EXPECT_EQ(actual[i].fans[j].type, expected[i].fans[j].type);
            EXPECT_EQ(actual[i].fans[j].inventory,
                      expected[i].fans[j].inventory);
            EXPECT_EQ(actual[i].fans[j].shortName,
                      expected[i].fans[j].shortName);
            EXPECT_EQ(actual[i].fans[j].gpioInfo.name,
                      expected[i].fans[j].gpioInfo.name);
            EXPECT_EQ(actual[i].fans[j].gpioInfo.isActiveLow,
                      expected[i].fans[j].gpioInfo.isActiveLow);
            EXPECT_EQ(actual[i].fans[j].gpioInfo.tachRedundancy,
                      expected[i].fans[j].gpioInfo.tachRedundancy);
            EXPECT_EQ(actual[i].fans[j].gpioInfo.fanPrettyName,
                      expected[i].fans[j].gpioInfo.fanPrettyName);
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
                    1,
                    2,
                    3,
                    4,
                    5,
                    6,
                    7,
                    8
                ],
                "zones": [
                    {
                        "name": "chassis_${chassis}_zone",
                        "fans": [
                            {
                                "type": "systemFan",
                                "inventory": "/system/chassis${chassis}/motherboard/fan0",
                                "short_name": "chassis${chassis}_fan0",
                                "pretty_name": "Node ${node} fan 0",
                                "presence_gpio": "fan0_chassis${chassis}_presence",
                                "presence_gpio_polarity": "Low"
                            },
                            {
                                "type": "systemFan",
                                "inventory": "/system/chassis${chassis}/motherboard/fan1",
                                "short_name": "chassis${chassis}_fan1",
                                "pretty_name": "Node ${node} fan 1",
                                "presence_gpio": "fan1_chassis${chassis}_presence",
                                "presence_gpio_polarity": "Low"
                            },
                            {
                                "type": "systemFan",
                                "inventory": "/system/chassis${chassis}/motherboard/fan2",
                                "short_name": "chassis${chassis}_fan2",
                                "pretty_name": "Node ${node} fan 2",
                                "presence_gpio": "fan2_chassis${chassis}_presence",
                                "presence_gpio_polarity": "Low"
                            },
                            {
                                "type": "systemFan",
                                "inventory": "/system/chassis${chassis}/motherboard/fan3",
                                "short_name": "chassis${chassis}_fan3",
                                "pretty_name": "Node ${node} fan 3",
                                "presence_gpio": "fan3_chassis${chassis}_presence",
                                "presence_gpio_polarity": "Low"
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
            .name = "chassis1",
            .zones = {ZoneDefinition{
                .name = "chassis_1_zone",
                .fans =
                    {FanAssignment{
                         .type = "systemFan",
                         .inventory = "/system/chassis1/motherboard/fan0",
                         .shortName = "chassis1_fan0",
                         .gpioInfo = GpioInfo{.name = "fan0_chassis1_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 0 fan 0"}},
                     FanAssignment{
                         .type = "systemFan",
                         .inventory = "/system/chassis1/motherboard/fan1",
                         .shortName = "chassis1_fan1",
                         .gpioInfo = GpioInfo{.name = "fan1_chassis1_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 0 fan 1"}},
                     FanAssignment{
                         .type = "systemFan",
                         .inventory = "/system/chassis1/motherboard/fan2",
                         .shortName = "chassis1_fan2",
                         .gpioInfo = GpioInfo{.name = "fan2_chassis1_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 0 fan 2"}},
                     FanAssignment{
                         .type = "systemFan",
                         .inventory =
                             "/system/chassis1/motherboard/fan3",
                         .shortName =
                             "chassis1_fan3",
                         .gpioInfo = GpioInfo{.name = "fan3_chassis1_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName =
                                                  "Node 0 fan 3"}}},
                .faultHandling = faultHandlingConfig}},
            .availPropUsed = true,
            .chassisNum = 1},
        ChassisDefinition{
            .name = "chassis2",
            .zones = {ZoneDefinition{
                .name = "chassis_2_zone",
                .fans =
                    {FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis2/motherboard/fan0",
                         .shortName =
                             "chassis2_fan0",
                         .gpioInfo = GpioInfo{.name = "fan0_chassis2_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 1 fan 0"}},
                     FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis2/motherboard/fan1",
                         .shortName =
                             "chassis2_fan1",
                         .gpioInfo = GpioInfo{.name = "fan1_chassis2_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 1 fan 1"}},
                     FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis2/motherboard/fan2",
                         .shortName =
                             "chassis2_fan2",
                         .gpioInfo = GpioInfo{.name = "fan2_chassis2_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 1 fan 2"}},
                     FanAssignment{.type = "systemFan",
                                   .inventory =
                                       "/system/chassis2/motherboard/fan3",
                                   .shortName = "chassis2_fan3",
                                   .gpioInfo =
                                       GpioInfo{.name =
                                                    "fan3_chassis2_presence",
                                                .isActiveLow = true,
                                                .tachRedundancy = true,
                                                .fanPrettyName =
                                                    "Node 1 fan 3"}}},
                .faultHandling = faultHandlingConfig}},
            .availPropUsed = true,
            .chassisNum = 2},
        ChassisDefinition{
            .name = "chassis3",
            .zones = {ZoneDefinition{
                .name = "chassis_3_zone",
                .fans =
                    {FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis3/motherboard/fan0",
                         .shortName =
                             "chassis3_fan0",
                         .gpioInfo = GpioInfo{.name = "fan0_chassis3_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 2 fan 0"}},
                     FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis3/motherboard/fan1",
                         .shortName =
                             "chassis3_fan1",
                         .gpioInfo = GpioInfo{.name = "fan1_chassis3_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 2 fan 1"}},
                     FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis3/motherboard/fan2",
                         .shortName =
                             "chassis3_fan2",
                         .gpioInfo = GpioInfo{.name = "fan2_chassis3_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 2 fan 2"}},
                     FanAssignment{.type = "systemFan",
                                   .inventory =
                                       "/system/chassis3/motherboard/fan3",
                                   .shortName = "chassis3_fan3",
                                   .gpioInfo =
                                       GpioInfo{.name =
                                                    "fan3_chassis3_presence",
                                                .isActiveLow = true,
                                                .tachRedundancy = true,
                                                .fanPrettyName =
                                                    "Node 2 fan 3"}}},
                .faultHandling = faultHandlingConfig}},
            .availPropUsed = true,
            .chassisNum = 3},
        ChassisDefinition{
            .name = "chassis4",
            .zones = {ZoneDefinition{
                .name = "chassis_4_zone",
                .fans =
                    {FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis4/motherboard/fan0",
                         .shortName =
                             "chassis4_fan0",
                         .gpioInfo = GpioInfo{.name = "fan0_chassis4_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 3 fan 0"}},
                     FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis4/motherboard/fan1",
                         .shortName =
                             "chassis4_fan1",
                         .gpioInfo = GpioInfo{.name = "fan1_chassis4_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 3 fan 1"}},
                     FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis4/motherboard/fan2",
                         .shortName =
                             "chassis4_fan2",
                         .gpioInfo = GpioInfo{.name = "fan2_chassis4_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 3 fan 2"}},
                     FanAssignment{.type = "systemFan",
                                   .inventory =
                                       "/system/chassis4/motherboard/fan3",
                                   .shortName = "chassis4_fan3",
                                   .gpioInfo =
                                       GpioInfo{.name =
                                                    "fan3_chassis4_presence",
                                                .isActiveLow = true,
                                                .tachRedundancy = true,
                                                .fanPrettyName =
                                                    "Node 3 fan 3"}}},
                .faultHandling = faultHandlingConfig}},
            .availPropUsed = true,
            .chassisNum = 4},
        ChassisDefinition{
            .name = "chassis5",
            .zones = {ZoneDefinition{
                .name = "chassis_5_zone",
                .fans =
                    {FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis5/motherboard/fan0",
                         .shortName =
                             "chassis5_fan0",
                         .gpioInfo = GpioInfo{.name = "fan0_chassis5_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 4 fan 0"}},
                     FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis5/motherboard/fan1",
                         .shortName =
                             "chassis5_fan1",
                         .gpioInfo = GpioInfo{.name = "fan1_chassis5_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 4 fan 1"}},
                     FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis5/motherboard/fan2",
                         .shortName =
                             "chassis5_fan2",
                         .gpioInfo = GpioInfo{.name = "fan2_chassis5_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 4 fan 2"}},
                     FanAssignment{.type = "systemFan",
                                   .inventory =
                                       "/system/chassis5/motherboard/fan3",
                                   .shortName = "chassis5_fan3",
                                   .gpioInfo =
                                       GpioInfo{.name =
                                                    "fan3_chassis5_presence",
                                                .isActiveLow = true,
                                                .tachRedundancy = true,
                                                .fanPrettyName =
                                                    "Node 4 fan 3"}}},
                .faultHandling = faultHandlingConfig}},
            .availPropUsed = true,
            .chassisNum = 5},
        ChassisDefinition{
            .name = "chassis6",
            .zones = {ZoneDefinition{
                .name = "chassis_6_zone",
                .fans =
                    {FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis6/motherboard/fan0",
                         .shortName =
                             "chassis6_fan0",
                         .gpioInfo = GpioInfo{.name = "fan0_chassis6_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 5 fan 0"}},
                     FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis6/motherboard/fan1",
                         .shortName =
                             "chassis6_fan1",
                         .gpioInfo = GpioInfo{.name = "fan1_chassis6_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 5 fan 1"}},
                     FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis6/motherboard/fan2",
                         .shortName =
                             "chassis6_fan2",
                         .gpioInfo = GpioInfo{.name = "fan2_chassis6_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 5 fan 2"}},
                     FanAssignment{.type = "systemFan",
                                   .inventory =
                                       "/system/chassis6/motherboard/fan3",
                                   .shortName = "chassis6_fan3",
                                   .gpioInfo =
                                       GpioInfo{.name =
                                                    "fan3_chassis6_presence",
                                                .isActiveLow = true,
                                                .tachRedundancy = true,
                                                .fanPrettyName =
                                                    "Node 5 fan 3"}}},
                .faultHandling = faultHandlingConfig}},
            .availPropUsed = true,
            .chassisNum = 6},
        ChassisDefinition{
            .name = "chassis7",
            .zones = {ZoneDefinition{
                .name = "chassis_7_zone",
                .fans =
                    {FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis7/motherboard/fan0",
                         .shortName =
                             "chassis7_fan0",
                         .gpioInfo = GpioInfo{.name = "fan0_chassis7_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 6 fan 0"}},
                     FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis7/motherboard/fan1",
                         .shortName =
                             "chassis7_fan1",
                         .gpioInfo = GpioInfo{.name = "fan1_chassis7_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 6 fan 1"}},
                     FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis7/motherboard/fan2",
                         .shortName =
                             "chassis7_fan2",
                         .gpioInfo = GpioInfo{.name = "fan2_chassis7_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 6 fan 2"}},
                     FanAssignment{.type = "systemFan",
                                   .inventory =
                                       "/system/chassis7/motherboard/fan3",
                                   .shortName = "chassis7_fan3",
                                   .gpioInfo =
                                       GpioInfo{.name =
                                                    "fan3_chassis7_presence",
                                                .isActiveLow = true,
                                                .tachRedundancy = true,
                                                .fanPrettyName =
                                                    "Node 6 fan 3"}}},
                .faultHandling = faultHandlingConfig}},
            .availPropUsed = true,
            .chassisNum = 7},
        ChassisDefinition{
            .name = "chassis8",
            .zones = {ZoneDefinition{
                .name = "chassis_8_zone",
                .fans =
                    {FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis8/motherboard/fan0",
                         .shortName =
                             "chassis8_fan0",
                         .gpioInfo = GpioInfo{.name = "fan0_chassis8_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 7 fan 0"}},
                     FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis8/motherboard/fan1",
                         .shortName =
                             "chassis8_fan1",
                         .gpioInfo = GpioInfo{.name = "fan1_chassis8_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 7 fan 1"}},
                     FanAssignment{
                         .type =
                             "systemFan",
                         .inventory =
                             "/system/chassis8/motherboard/fan2",
                         .shortName =
                             "chassis8_fan2",
                         .gpioInfo = GpioInfo{.name = "fan2_chassis8_presence",
                                              .isActiveLow = true,
                                              .tachRedundancy = true,
                                              .fanPrettyName = "Node 7 fan 2"}},
                     FanAssignment{.type = "systemFan",
                                   .inventory =
                                       "/system/chassis8/motherboard/fan3",
                                   .shortName = "chassis8_fan3",
                                   .gpioInfo =
                                       GpioInfo{.name =
                                                    "fan3_chassis8_presence",
                                                .isActiveLow = true,
                                                .tachRedundancy = true,
                                                .fanPrettyName =
                                                    "Node 7 fan 3"}}},
                .faultHandling = faultHandlingConfig}},
            .availPropUsed = true,
            .chassisNum = 8}};

    ASSERT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < actual.size(); i++)
    {
        EXPECT_EQ(actual[i].name, expected[i].name);
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
                EXPECT_EQ(actual[i].zones[j].fans[k].inventory,
                          expected[i].zones[j].fans[k].inventory);
                EXPECT_EQ(actual[i].zones[j].fans[k].shortName,
                          expected[i].zones[j].fans[k].shortName);
                EXPECT_EQ(actual[i].zones[j].fans[k].gpioInfo.name,
                          expected[i].zones[j].fans[k].gpioInfo.name);
                EXPECT_EQ(actual[i].zones[j].fans[k].gpioInfo.isActiveLow,
                          expected[i].zones[j].fans[k].gpioInfo.isActiveLow);
                EXPECT_EQ(actual[i].zones[j].fans[k].gpioInfo.tachRedundancy,
                          expected[i].zones[j].fans[k].gpioInfo.tachRedundancy);
                EXPECT_EQ(actual[i].zones[j].fans[k].gpioInfo.fanPrettyName,
                          expected[i].zones[j].fans[k].gpioInfo.fanPrettyName);
            }
            EXPECT_EQ(actual[i].zones[j].faultHandling,
                      expected[i].zones[j].faultHandling);
        }
        EXPECT_EQ(actual[i].availPropUsed, expected[i].availPropUsed);
        EXPECT_EQ(actual[i].chassisNum, expected[i].chassisNum);
    }
}

TEST(MultiChassisJsonParserTest, VarExpansionTest)
{
    const auto chassisConfig = R"({
        "chassis_definitions": [
            {
                "name": "chassis${chassis}_node${node}",
                "availPropUsed": true,
                "chassis_numbers": [
                    1,
                    2
                ],
                "zones": [
                    {
                        "name": "zone_${node}_chassis_${chassis}_${node}",
                        "fans": [
                            {
                                "type": "systemFan",
                                "inventory": "/system/chassis${chassis}_node${node}/motherboard/fan0",
                                "short_name": "chassis${chassis}_node${node}_fan0_chassis${chassis}",
                                "pretty_name": "Node ${node} fan 0",
                                "presence_gpio": "fan0_chassis${chassis}_node${node}_presence",
                                "presence_gpio_polarity": "Low"
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
            .name = "chassis1_node0",
            .zones = {ZoneDefinition{
                .name = "zone_0_chassis_1_0",
                .fans = {FanAssignment{
                    .type = "systemFan",
                    .inventory = "/system/chassis1_node0/motherboard/fan0",
                    .shortName = "chassis1_node0_fan0_chassis1",
                    .gpioInfo = GpioInfo{.name = "fan0_chassis1_node0_presence",
                                         .isActiveLow = true,
                                         .tachRedundancy = true,
                                         .fanPrettyName = "Node 0 fan 0"}}},
                .faultHandling = faultHandlingConfig}},
            .availPropUsed = true,
            .chassisNum = 1},
        ChassisDefinition{
            .name = "chassis2_node1",
            .zones = {ZoneDefinition{
                .name = "zone_1_chassis_2_1",
                .fans = {FanAssignment{
                    .type = "systemFan",
                    .inventory = "/system/chassis2_node1/motherboard/fan0",
                    .shortName = "chassis2_node1_fan0_chassis2",
                    .gpioInfo = GpioInfo{.name = "fan0_chassis2_node1_presence",
                                         .isActiveLow = true,
                                         .tachRedundancy = true,
                                         .fanPrettyName = "Node 1 fan 0"}}},
                .faultHandling = faultHandlingConfig}},
            .availPropUsed = true,
            .chassisNum = 2}};

    ASSERT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < actual.size(); i++)
    {
        EXPECT_EQ(actual[i].name, expected[i].name);
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
                EXPECT_EQ(actual[i].zones[j].fans[k].inventory,
                          expected[i].zones[j].fans[k].inventory);
                EXPECT_EQ(actual[i].zones[j].fans[k].shortName,
                          expected[i].zones[j].fans[k].shortName);
                EXPECT_EQ(actual[i].zones[j].fans[k].gpioInfo.name,
                          expected[i].zones[j].fans[k].gpioInfo.name);
                EXPECT_EQ(actual[i].zones[j].fans[k].gpioInfo.isActiveLow,
                          expected[i].zones[j].fans[k].gpioInfo.isActiveLow);
                EXPECT_EQ(actual[i].zones[j].fans[k].gpioInfo.tachRedundancy,
                          expected[i].zones[j].fans[k].gpioInfo.tachRedundancy);
                EXPECT_EQ(actual[i].zones[j].fans[k].gpioInfo.fanPrettyName,
                          expected[i].zones[j].fans[k].gpioInfo.fanPrettyName);
            }
            EXPECT_EQ(actual[i].zones[j].faultHandling,
                      expected[i].zones[j].faultHandling);
        }
        EXPECT_EQ(actual[i].availPropUsed, expected[i].availPropUsed);
        EXPECT_EQ(actual[i].chassisNum, expected[i].chassisNum);
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
                        "inventory": "/system/chassis${chassis}/motherboard/fan0",
                        "short_name": "chassis${chassis}_fan0",
                        "pretty_name": "Node ${node} fan 0",
                        "presence_gpio": "fan0_chassis${chassis}_presence",
                        "presence_gpio_polarity": "Low"
                    },
                    {
                        "type": "systemFan",
                        "inventory": "/system/chassis${chassis}/motherboard/fan1",
                        "short_name": "chassis${chassis}_fan1",
                        "pretty_name": "Node ${node} fan 1",
                        "presence_gpio": "fan1_chassis${chassis}_presence",
                        "presence_gpio_polarity": "Low"
                    },
                    {
                        "type": "systemFan",
                        "inventory": "/system/chassis${chassis}/motherboard/fan2",
                        "short_name": "chassis${chassis}_fan2",
                        "pretty_name": "Node ${node} fan 2",
                        "presence_gpio": "fan2_chassis${chassis}_presence",
                        "presence_gpio_polarity": "Low"
                    },
                    {
                        "type": "systemFan",
                        "inventory": "/system/chassis${chassis}/motherboard/fan3",
                        "short_name": "chassis${chassis}_fan3",
                        "pretty_name": "Node ${node} fan 3",
                        "presence_gpio": "fan3_chassis${chassis}_presence",
                        "presence_gpio_polarity": "Low"
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
