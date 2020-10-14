#pragma once

#include "logging.hpp"
#include "power_interface.hpp"

#include <fmt/format.h>

#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <chrono>

namespace phosphor::fan::monitor
{

/**
 * @class PowerOffAction
 *
 * This is the base class for a power off action, which is
 * used by the PowerOffRule class to do different types of
 * power offs based on fan failures.
 *
 * The power off is started with the start() method, and the
 * derived class may or may not allow it to be stopped with
 * the cancel() method, which is really only useful when
 * there is a delay before the power off.
 *
 * It uses the PowerInterfaceBase object pointer to perform
 * the D-Bus call to do the power off, so it can be mocked
 * for testing.
 */
class PowerOffAction
{
  public:
    PowerOffAction() = delete;
    virtual ~PowerOffAction() = default;
    PowerOffAction(const PowerOffAction&) = delete;
    PowerOffAction& operator=(const PowerOffAction&) = delete;
    PowerOffAction(PowerOffAction&&) = delete;
    PowerOffAction& operator=(PowerOffAction&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] name - The action name.  Used for tracing.
     * powerInterface - The object used to invoke the power off.
     */
    PowerOffAction(const std::string& name,
                   std::shared_ptr<PowerInterfaceBase> powerInterface) :
        _name(name),
        _powerIface(std::move(powerInterface)),
        _event(sdeventplus::Event::get_default())
    {}

    /**
     * @brief Starts the power off.
     *
     * Though this occurs in the child class, usually this
     * involves starting a timer and then powering off when it
     * times out.
     */
    virtual void start() = 0;

    /**
     * @brief Attempts to cancel the power off, if the derived
     *        class allows it, and assuming the power off hasn't
     *        already happened.
     *
     * The 'force' parameter is mainly for use when something else
     * powered off the system so this action doesn't need to run
     * anymore even if it isn't usually cancelable.
     *
     * @param[in] force - If the cancel should be forced
     *
     * @return bool - If the cancel was allowed/successful
     */
    virtual bool cancel(bool force) = 0;

    /**
     * @brief If the power off action is currently in progress, which
     *        usually means it's still in the delay time before the
     *        power off D-Bus command is executed.
     *
     * @return bool - If the action is active
     */
    bool active() const
    {
        return _active;
    }

    /**
     * @brief Returns the name of the action
     *
     * @return const std::string& - The name
     */
    const std::string& name() const
    {
        return _name;
    }

  protected:
    /**
     * @brief The name of the action, which is set by the
     *        derived class.
     */
    const std::string _name;

    /**
     * @brief If the action is currently active or not.
     */
    bool _active = false;

    /**
     * @brief The object used to invoke the power off with.
     */
    std::shared_ptr<PowerInterfaceBase> _powerIface;

    /**
     * @brief The event loop object. Needed by timers.
     */
    sdeventplus::Event _event;
};

/**
 * @class HardPowerOff
 *
 * This class is derived from the PowerOffAction class
 * and will execute a hard power off after some delay.
 */
class HardPowerOff : public PowerOffAction
{
  public:
    HardPowerOff() = delete;
    ~HardPowerOff() = default;
    HardPowerOff(const HardPowerOff&) = delete;
    HardPowerOff& operator=(const HardPowerOff&) = delete;
    HardPowerOff(HardPowerOff&&) = delete;
    HardPowerOff& operator=(HardPowerOff&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] delay - The amount of time in seconds to wait before
     *                    doing the power off
     * @param[in] powerInterface - The object to use to do the power off
     */
    HardPowerOff(uint32_t delay,
                 std::shared_ptr<PowerInterfaceBase> powerInterface) :
        PowerOffAction("Hard Power Off: " + std::to_string(delay) + "s",
                       powerInterface),
        _delay(delay),
        _timer(_event, std::bind(std::mem_fn(&HardPowerOff::powerOff), this))
    {}

    /**
     * @brief Starts a timer upon the expiration of which the
     *        hard power off will be done.
     */
    void start() override
    {
        _timer.restartOnce(_delay);
    }

    /**
     * @brief Cancels the timer.  This is always allowed.
     *
     * @param[in] force - If the cancel should be forced or not
     *                    (not checked in this case)
     * @return bool - Always returns true
     */
    bool cancel(bool) override
    {
        if (_timer.isEnabled())
        {
            _timer.setEnabled(false);
        }

        // Can always be canceled
        return true;
    }

    /**
     * @brief Performs the hard power off.
     */
    void powerOff()
    {
        getLogger().log(
            fmt::format("Action '{}' executing hard power off", name()));
        _powerIface->hardPowerOff();
    }

  private:
    /**
     * @brief The number of seconds to wait between starting the
     *        action and doing the power off.
     */
    std::chrono::seconds _delay;

    /**
     * @brief The Timer object used to handle the delay.
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> _timer;
};

/**
 * @class SoftPowerOff
 *
 * This class is derived from the PowerOffAction class
 * and will execute a soft power off after some delay.
 */
class SoftPowerOff : public PowerOffAction
{
  public:
    SoftPowerOff() = delete;
    ~SoftPowerOff() = default;
    SoftPowerOff(const SoftPowerOff&) = delete;
    SoftPowerOff& operator=(const SoftPowerOff&) = delete;
    SoftPowerOff(SoftPowerOff&&) = delete;
    SoftPowerOff& operator=(SoftPowerOff&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] delay - The amount of time in seconds to wait before
     *                    doing the power off
     * @param[in] powerInterface - The object to use to do the power off
     */
    SoftPowerOff(uint32_t delay,
                 std::shared_ptr<PowerInterfaceBase> powerInterface) :
        PowerOffAction("Soft Power Off: " + std::to_string(delay) + "s",
                       powerInterface),
        _delay(delay),
        _timer(_event, std::bind(std::mem_fn(&SoftPowerOff::powerOff), this))
    {}

    /**
     * @brief Starts a timer upon the expiration of which the
     *        soft power off will be done.
     */
    void start() override
    {
        _timer.restartOnce(_delay);
    }

    /**
     * @brief Cancels the timer.  This is always allowed.
     *
     * @param[in] force - If the cancel should be forced or not
     *                    (not checked in this case)
     * @return bool - Always returns true
     */
    bool cancel(bool) override
    {
        if (_timer.isEnabled())
        {
            _timer.setEnabled(false);
        }

        // Can always be canceled
        return true;
    }

    /**
     * @brief Performs the soft power off.
     */
    void powerOff()
    {
        getLogger().log(
            fmt::format("Action '{}' executing soft power off", name()));
        _powerIface->softPowerOff();
    }

  private:
    /**
     * @brief The number of seconds to wait between starting the
     *        action and doing the power off.
     */
    std::chrono::seconds _delay;

    /**
     * @brief The Timer object used to handle the delay.
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> _timer;
};

/**
 * @class EpowPowerOff
 *
 * Still TODO, but has a cancelable service mode delay followed
 * by an uncancelable meltdown delay followed by a hard power off, with
 * some sort of EPOW alert in there as well.
 */
class EpowPowerOff : public PowerOffAction
{
  public:
    EpowPowerOff() = delete;
    ~EpowPowerOff() = default;
    EpowPowerOff(const EpowPowerOff&) = delete;
    EpowPowerOff& operator=(const EpowPowerOff&) = delete;
    EpowPowerOff(EpowPowerOff&&) = delete;
    EpowPowerOff& operator=(EpowPowerOff&&) = delete;

    EpowPowerOff(uint32_t serviceModeDelay, uint32_t meltdownDelay,
                 std::shared_ptr<PowerInterfaceBase> powerInterface) :
        PowerOffAction("EPOW Power Off: " + std::to_string(serviceModeDelay) +
                           "s/" + std::to_string(meltdownDelay) + "s",
                       powerInterface),
        _serviceModeDelay(serviceModeDelay), _meltdownDelay(meltdownDelay)
    {}

    void start() override
    {
        // TODO
    }

    bool cancel(bool) override
    {
        // TODO
        return true;
    }

  private:
    std::chrono::seconds _serviceModeDelay;
    std::chrono::seconds _meltdownDelay;
};
} // namespace phosphor::fan::monitor
