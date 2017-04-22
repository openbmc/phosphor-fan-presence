#pragma once

#include <map>
#include <set>
#include <tuple>
#include "fan_properties.hpp"

extern const std::map<std::string,
                      std::set<phosphor::fan::Properties>> fanDetectMap;
