// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include "tach_sensor.hpp"
#include "types.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/lg2.hpp>
#include <xyz/openbmc_project/Object/Enable/server.hpp>

#include <functional>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace phosphor::fan::monitor::multi_chassis
{
struct SensorDefinition
{
    std::string pathSuffix;
    bool hasTarget;
    std::string targetInterface;
    std::string targetPath;
    double factor;
    int64_t offset;
    size_t threshold;
    bool ignoreAboveMax;
};
struct FanTypeDefinition
{
    std::string type;
    size_t method;
    size_t funcDelay;
    size_t timeout;
    size_t deviation;
    size_t upperDeviation;
    size_t numSensorFailsForNonfunc;
    size_t monitorStartDelay;
    size_t countInterval;
    std::optional<size_t> nonfuncRotorErrDelay;
    std::optional<size_t> fanMissingErrDelay;
    std::vector<SensorDefinition> sensorList;
    std::optional<Condition> condition;
    bool funcOnPresent;
};
struct FanAssignment
{
    std::string type;
    std::string inventoryBase;
    std::string shortName;
};
struct ZoneDefinition
{
    std::string name;
    std::vector<FanAssignment> fans;
    nlohmann::json faultHandling;
};
struct ChassisDefinition
{
    std::vector<int> chassisNumbers;
    std::vector<ZoneDefinition> zones;
    bool available;
};
} // namespace phosphor::fan::monitor::multi_chassis
