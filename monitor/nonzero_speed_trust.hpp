#pragma once

#include "trust_group.hpp"

namespace phosphor
{
namespace fan
{
namespace trust
{

/**
 * @class NonzeroSpeed
 *
 * A trust group where the sensors in the group are trusted as long
 * as at least one of them has a nonzero speed.  If all sensors
 * have a speed of zero, then no sensor in the group is trusted.
 */
class NonzeroSpeed : public Group
{
    public:

        NonzeroSpeed() = delete;
        ~NonzeroSpeed() = default;
        NonzeroSpeed(const NonzeroSpeed&) = delete;
        NonzeroSpeed& operator=(const NonzeroSpeed&) = delete;
        NonzeroSpeed(NonzeroSpeed&&) = delete;
        NonzeroSpeed& operator=(NonzeroSpeed&&) = delete;

        /**
         * Constructor
         *
         * @param[in] names - the names of the sensors in the group
         */
        NonzeroSpeed(const std::vector<std::string>& names) :
                Group(names)
        {
        }

        /**
         * Determines if the group is trusted by checking
         * if any sensor has a nonzero speed.  If all speeds
         * are zero, then no sensors in the group are trusted.
         *
         * @param[in] sensor - the TachSensor object
         */
        bool checkGroupTrust(
                const monitor::TachSensor& sensor) override
        {
            auto trusted = std::any_of(
                    _sensors.begin(),
                    _sensors.end(),
                    [](const auto& s)
                    {
                        return s.get()->getInput() != 0;
                    });

            return trusted;
        }
};

}
}
}
