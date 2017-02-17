#pragma once

#include <string>
#include <vector>


namespace phosphor
{
namespace fan
{

    struct Properties
    {
        std::string inv;
        std::string desc;
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

}
}
