#pragma once

#include "fan.hpp"
#include "power_state.hpp"
#include "rpolicy.hpp"

#include <sdeventplus/utility/timer.hpp>

#include <functional>
#include <vector>

namespace phosphor
{
namespace fan
{
namespace presence
{

class PresenceSensor;

/**
 * @class AnyOf
 * @brief AnyOf redundancy policy.
 *
 * The any of redundancy policy monitors all sensor
 * states in the redundancy set and reports true when any
 * sensor in the set reports true.
 */
class AnyOf : public RedundancyPolicy
{
  public:
    AnyOf() = delete;
    AnyOf(const AnyOf&) = default;
    AnyOf& operator=(const AnyOf&) = default;
    AnyOf(AnyOf&&) = default;
    AnyOf& operator=(AnyOf&&) = default;
    ~AnyOf() = default;

    /**
     * @brief Construct an any of bitwise policy.
     *
     * @param[in] fan - The fan associated with the policy.
     * @param[in] s - The set of sensors associated with the policy.
     * @param[in] e - EEPROM device instance
     */
    AnyOf(const Fan& fan,
          const std::vector<std::reference_wrapper<PresenceSensor>>& s,
          std::unique_ptr<EEPROMDevice> e);

    /**
     * @brief Construct an any of bitwise policy.
     *
     * @param[in] fan - The fan associated with the policy.
     * @param[in] s - The set of sensors associated with the policy.
     */
    AnyOf(const Fan& fan,
          const std::vector<std::reference_wrapper<PresenceSensor>>& s) :
        AnyOf(fan, s, nullptr)
    {}

    /**
     * @brief stateChanged
     *
     * Update the inventory and execute the fallback
     * policy.
     *
     * @param[in] present - The new presence state according
     *             to the specified sensor.
     * @param[in] sensor - The sensor reporting the new state.
     */
    void stateChanged(bool present, PresenceSensor& sensor) override;

    /**
     * @brief monitor
     *
     * Start monitoring the fan.
     */
    void monitor() override;

  private:
    /**
     * @brief Checks that the sensors contained in this policy all
     *        agree on the presence value.  If they don't, then call
     *        logConflict() on the sensors that don't think the fan
     *        is present as they may be broken.
     *
     * This check will only happen when power is on.
     */
    void checkSensorConflicts();

    /**
     * @brief Callback function called after a post-poweron delay.
     *
     * The _powerOnDelayTimer is started when _powerState says the
     * power is on to give fans a bit of time to spin up so tachs
     * wouldn't be zero.  This is the callback function for that timer.
     *
     * It will call checkSensorConflicts().
     */
    void delayedAfterPowerOn();

    /**
     * @brief Says if power is on, though not until the post
     *        power on delay is complete.
     *
     * @return bool - if power is on.
     */
    inline bool isPowerOn() const
    {
        return _powerOn;
    }

    /**
     * @brief Called by the PowerState object when the power
     *        state changes.
     *
     * When power changes to on:
     *   - Clears the memory of previous sensor conflicts.
     *   - Starts the post power on delay timer.
     *
     * @param[in] powerOn - If power is now on or off.
     */
    void powerStateChanged(bool powerOn);

    static constexpr size_t sensorPos = 0;
    static constexpr size_t presentPos = 1;
    static constexpr size_t conflictPos = 2;

    /**
     * @brief All presence sensors in the redundancy set.
     *
     * Each entry contains:
     *  - A reference to a PresenceSensor
     *  - The current presence state
     *  - If the sensors have logged conflicts in their answers.
     */
    std::vector<std::tuple<std::reference_wrapper<PresenceSensor>, bool, bool>>
        state;

    /**
     * @brief Pointer to the PowerState object used to track power changes.
     */
    std::shared_ptr<PowerState> _powerState;

    /**
     * @brief Post power on delay timer, where the conflict checking code
     *        doesn't consider power on until this timer expires.
     *
     * This gives fans a chance to start spinning before checking them.
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>
        _powerOnDelayTimer;

    /**
     * @brief Current power state.
     */
    bool _powerOn;
};

} // namespace presence
} // namespace fan
} // namespace phosphor
