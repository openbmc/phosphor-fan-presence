#pragma once

#include "types.hpp"

#include <algorithm>
#include <vector>

namespace phosphor::fan::monitor
{

/**
 * @class PowerOffCause
 *
 * This abstract class provides a satisfied() pure virtual method
 * that is called to know if the system should be powered off due
 * to fan health.  Each type of class that is derived from this
 * one provides different behavior, for example one may count
 * missing fans, and another may count nonfunctional fans.
 */
class PowerOffCause
{
  public:
    PowerOffCause() = delete;
    virtual ~PowerOffCause() = default;
    PowerOffCause(const PowerOffCause&) = delete;
    PowerOffCause& operator=(const PowerOffCause&) = delete;
    PowerOffCause(PowerOffCause&&) = delete;
    PowerOffCause& operator=(PowerOffCause&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] count - The number of items that is compared
     *                    against in the derived class.
     * @param[in] name - The name of the cause. Used for tracing.
     */
    PowerOffCause(size_t count, const std::string& name) :
        _count(count), _name(std::to_string(count) + " " + name)
    {}

    /**
     * @brief Pure virtual that says if the system should be powered
     *        off based on the fan health.
     *
     * @param[in] fanHealth - The FanHealth map
     *
     * @return bool - If system should be powered off
     */
    virtual bool satisfied(const FanHealth& fanHealth) = 0;

    /**
     * @brief Returns the name of the cause.
     *
     * For example:  "3 Missing Fans"
     *
     * @return std::string - The name
     */
    const std::string& name() const
    {
        return _name;
    }

  protected:
    /**
     * @brief The number of fan health items that the derived
     *        class uses to compare to the fan health status.
     *        For example, a 3 for 3 missing fans.
     */
    const size_t _count;

    /**
     * @brief The cause name
     */
    const std::string _name;
};

/**
 * @class MissingFanFRUCause
 *
 * This class provides a satisfied() method that checks for
 * missing fans in the fan health map.
 *
 */
class MissingFanFRUCause : public PowerOffCause
{
  public:
    MissingFanFRUCause() = delete;
    ~MissingFanFRUCause() = default;
    MissingFanFRUCause(const MissingFanFRUCause&) = delete;
    MissingFanFRUCause& operator=(const MissingFanFRUCause&) = delete;
    MissingFanFRUCause(MissingFanFRUCause&&) = delete;
    MissingFanFRUCause& operator=(MissingFanFRUCause&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] count - The minimum number of fans that must be
     *                    missing to need a power off.
     */
    explicit MissingFanFRUCause(size_t count) :
        PowerOffCause(count, "Missing Fan FRUs")
    {}

    /**
     * @brief Returns true if 'count' or more fans are missing
     *        to require a power off.
     *
     * @param[in] fanHealth - The FanHealth map
     */
    bool satisfied(const FanHealth& fanHealth) override
    {
        size_t count = std::count_if(
            fanHealth.begin(), fanHealth.end(), [](const auto& fan) {
                return !std::get<presentHealthPos>(fan.second);
            });

        return count >= _count;
    }
};

/**
 * @class NonfuncFanRotorCause
 *
 * This class provides a satisfied() method that checks for
 * nonfunctional fan rotors in the fan health map.
 */
class NonfuncFanRotorCause : public PowerOffCause
{
  public:
    NonfuncFanRotorCause() = delete;
    ~NonfuncFanRotorCause() = default;
    NonfuncFanRotorCause(const NonfuncFanRotorCause&) = delete;
    NonfuncFanRotorCause& operator=(const NonfuncFanRotorCause&) = delete;
    NonfuncFanRotorCause(NonfuncFanRotorCause&&) = delete;
    NonfuncFanRotorCause& operator=(NonfuncFanRotorCause&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] count - The minimum number of rotors that must be
     *                    nonfunctional nonfunctional to need a power off.
     */
    explicit NonfuncFanRotorCause(size_t count) :
        PowerOffCause(count, "Nonfunctional Fan Rotors")
    {}

    /**
     * @brief Returns true if 'count' or more rotors are nonfunctional
     *        to require a power off.
     *
     * @param[in] fanHealth - The FanHealth map
     */
    bool satisfied(const FanHealth& fanHealth) override
    {
        size_t count = std::accumulate(
            fanHealth.begin(), fanHealth.end(), 0,
            [](int sum, const auto& fan) {
                const auto& tachs = std::get<sensorFuncHealthPos>(fan.second);
                auto nonFuncTachs =
                    std::count_if(tachs.begin(), tachs.end(),
                                  [](bool tach) { return !tach; });
                return sum + nonFuncTachs;
            });

        return count >= _count;
    }
};

} // namespace phosphor::fan::monitor
