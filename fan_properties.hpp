#pragma once

#include <string>
#include <vector>


namespace phosphor
{
namespace fan
{

/**
 * @brief Fan enclosure properties
 */
struct Properties
{
    /** @brief Inventory path for a fan enclosure */
    std::string inv;
    /** @brief Description for a fan enclosure added to inventory object */
    std::string desc;
    /** @brief List of sensors associated with a fan enclosure */
    std::vector<std::string> sensors;

    // Needed for inserting elements into sets
    bool operator<(const Properties& right) const
    {
        if (inv == right.inv)
        {
            return sensors < right.sensors;
        }
        return inv < right.inv;
    }
};

} // namespace fan
} // namespace phosphor
