#pragma once

#include <memory>
#include <vector>
#include "tach_sensor.hpp"
#include "trust_group.hpp"
#include "types.hpp"

namespace phosphor
{
namespace fan
{
namespace trust
{

/**
 * @class Manager
 *
 * The sensor trust manager class.  It can be asked if a tach sensor's
 * reading can be trusted or not, based on the trust groups the sensor
 * is in.  It also handles starting and stopping timers for group
 * sensors, to be used when the trust status changes.
 *
 * See the trust::Group documentation for more details on sensor trust.
 */
class Manager
{
    public:

        Manager() = delete;
        Manager(const Manager&) = delete;
        Manager& operator=(const Manager&) = delete;
        Manager(Manager&&) = default;
        Manager& operator=(Manager&&) = default;
        ~Manager() = default;

        /**
         * Constructor
         *
         * @param[in] functions - trust group creation function vector
         */
        Manager(const std::vector<monitor::CreateGroupFunction>& functions)
        {
            for (auto& create : functions)
            {
                groups.emplace_back(create());
            }
        }

        /**
         * Says if trust groups have been created and
         * need to be checked.
         *
         * @return bool - If there are any trust groups
         */
        inline bool active() const
        {
            return !groups.empty();
        }

        /**
         * Checks if a sensor value can be trusted
         *
         * Checks if the sensor is trusted in each group
         * it belongs to.  If it isn't trusted in any of
         * them, then it is considered untrusted.
         *
         * Also checks if the trust status just changed
         * in any groups that sensor belongs to.  If it
         * changed in any of them, then it returns that
         * it has changed.
         *
         * @param[in] sensor - the sensor to check
         *
         * @return tuple<bool, bool> -
         *     field 0: if sensor is trusted in all groups
         *     field 1: if trust status just changed in any groups
         */
        auto checkTrust(
                const monitor::TachSensor& sensor)
        {
            auto trusted = true;
            auto changed = false;

            for (auto& group : groups)
            {
                if (group->inGroup(sensor))
                {
                    auto answer = group->checkTrust(sensor);

                    if (!std::get<0>(answer))
                    {
                        trusted = false;
                    }

                    if (std::get<1>(answer))
                    {
                        changed = true;
                    }
                }
            }

            return std::tuple<bool, bool>(trusted, changed);
        }

        /**
         * Registers a sensor with any trust groups that are interested
         *
         * @param[in] sensor - the sensor to register
         */
        void registerSensor(
                std::unique_ptr<monitor::TachSensor>& sensor)
        {
            std::for_each(
                    groups.begin(),
                    groups.end(),
                    [&sensor](auto& group)
                    {
                        group->registerSensor(sensor);
                    });
        }

        /**
         * Stops timers for all sensors in the same trust group(s)
         * as this sensor when the group(s) just changed to untrusted.
         *
         * Ensures fans won't get made nonfunctional when a sensor
         * value isn't trusted.
         *
         * @param[in] sensor - the sensor in question
         */
        void stopUntrustedTimers(monitor::TachSensor& sensor)
        {
            std::for_each(
                    groups.begin(),
                    groups.end(),
                    [&sensor](const auto& group)
                    {
                        if (group->inGroup(sensor) &&
                            !group->getTrust() &&
                            group->trustChanged())
                        {
                            group->stopTimers();
                        }
                    });
        }

        /**
         * Starts all timers in the same trust group(s) as this
         * sensor when the group(s) just changed to trusted.
         *
         * Ensures that when a group goes from untrusted->trusted
         * all of its sensors will have to check in with a valid
         * value still.
         *
         * @param[in] sensor - the sensor in question
         */
        void startTrustedTimers(monitor::TachSensor& sensor)
        {
            std::for_each(
                    groups.begin(),
                    groups.end(),
                    [&sensor](const auto& group)
                    {
                        if (group->inGroup(sensor) &&
                            group->getTrust() &&
                            group->trustChanged())
                        {
                            group->startTimers();
                        }
                    });
        }

    private:

        /**
         * The list of sensor trust groups
         */
        std::vector<std::unique_ptr<Group>> groups;
};

}
}
}
