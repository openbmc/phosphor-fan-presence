#pragma once
#include "types.hpp"
#include <iostream> //FIXME

namespace phosphor
{
namespace fan
{
namespace control
{
namespace condition
{

constexpr auto eventConditionTypePos = 0;
//constexpr auto conditionPropertyListPos = 1;
/**
 * Check if a condition is true. Conditions are used to determine
 * which fan zone to use.
 *
 * @param[in] bus       - The D-Bus bus object
 * @param[in] condition - The condition to check if true
 * @return result       - True if the condition is true
 */
bool checkEventCondition(sdbusplus::bus::bus& bus, const auto& c)
{
    std::cout << "checkEventCondition\n";//FIXME
    auto& type = std::get<eventConditionTypePos>(c);
    if (type.compare("none") == 0)
    {
        return true;
    }

    auto& properties = std::get<conditionPropertyListPos>(c);

    for (auto& p : properties)
    {
        bool value = std::get<propertyValuePos>(p);
        bool propertyValue;

        // TODO: Support more types than just getProperty?
        if (type.compare("getProperty") == 0)
        {
            std::cout << "type is getProperty\n";
            std::cout << "propertyPathPos: " << std::get<propertyPathPos>(p) << '\n';
            std::cout << "propertyInterfacePos: " << std::get<propertyInterfacePos>(p) << '\n';
            std::cout << "propertyNamePos: " << std::get<propertyNamePos>(p) << '\n';
            phosphor::fan::util::getProperty(bus,
                        std::get<propertyPathPos>(p),
                        std::get<propertyInterfacePos>(p),
                        std::get<propertyNamePos>(p),
                        propertyValue);

            if (value != propertyValue)
            {
                return false;
            }
        }
    }
    return true;
}

}
}
}
}
