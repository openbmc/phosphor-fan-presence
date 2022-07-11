#pragma once

#include "logging.hpp"
#include "power_interface.hpp"
#include "sdbusplus.hpp"

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
    using PrePowerOffFunc = std::function<void()>;

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
     * @param[in] powerInterface - The object used to invoke the power off.
     * @param[in] powerOffFunc - A function to call right before the power
     *                           off occurs (after any delays).  May be
     *                           empty if no function is necessary.
     */
    PowerOffAction(const std::string& name,
                   std::shared_ptr<PowerInterfaceBase> powerInterface,
                   PrePowerOffFunc& powerOffFunc) :
        _name(name),
        _powerIface(std::move(powerInterface)),
        _event(sdeventplus::Event::get_default()),
        _prePowerOffFunc(powerOffFunc)
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
     * @brief Create a BMC Dump
     */
    void createBmcDump() const
    {
        try
        {
            util::SDBusPlus::callMethod(
                "xyz.openbmc_project.Dump.Manager",
                "/xyz/openbmc_project/dump/bmc",
                "xyz.openbmc_project.Dump.Create", "CreateDump",
                std::vector<std::pair<std::string,
                                      std::variant<std::string, uint64_t>>>());
        }
        catch (const std::exception& e)
        {
            getLogger().log(
                fmt::format("Caught exception while creating BMC dump: {}",
                            e.what()),
                Logger::error);
        }
    }

    /**
     * @brief The name of the action, which is set by the
     *        derived class.
     */
    const std::string _name;

    /**
     * @brief The object used to invoke the power off with.
     */
    std::shared_ptr<PowerInterfaceBase> _powerIface;

    /**
     * @brief The event loop object. Needed by timers.
     */
    sdeventplus::Event _event;

    /**
     * @brief A function that will be called right before
     *        the power off.
     */
    PrePowerOffFunc _prePowerOffFunc;
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
     * @param[in] func - A function to call right before the power
     *                   off occurs (after the delay).  May be
     *                   empty if no function is necessary.
     */
    HardPowerOff(uint32_t delay,
                 std::shared_ptr<PowerInterfaceBase> powerInterface,
                 PrePowerOffFunc func) :
        PowerOffAction("Hard Power Off: " + std::to_string(delay) + "s",
                       powerInterface, func),
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
        if (_prePowerOffFunc)
        {
            _prePowerOffFunc();
        }

        getLogger().log(
            fmt::format("Action '{}' executing hard power off", name()));
        _powerIface->hardPowerOff();

        createBmcDump();
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
     * @param[in] func - A function to call right before the power
     *                   off occurs (after the delay).  May be
     *                   empty if no function is necessary.
     */
    SoftPowerOff(uint32_t delay,
                 std::shared_ptr<PowerInterfaceBase> powerInterface,
                 PrePowerOffFunc func) :
        PowerOffAction("Soft Power Off: " + std::to_string(delay) + "s",
                       powerInterface, func),
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
        if (_prePowerOffFunc)
        {
            _prePowerOffFunc();
        }

        getLogger().log(
            fmt::format("Action '{}' executing soft power off", name()));
        _powerIface->softPowerOff();

        createBmcDump();
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
 * This class is derived from the PowerOffAction class and does the following:
 * 1) On start, the service mode timer is started.  This timer can be
 *    canceled if the cause is no longer satisfied (fans work again).
 * 2) When this timer expires:
 *   a) The thermal alert D-Bus property is set, this can be used as
 *      an EPOW alert to the host that a power off is imminent.
 *   b) The meltdown timer is started.  This timer cannot be canceled,
 *      and on expiration a hard power off occurs.
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

    /**
     * @brief Constructor
     *
     * @param[in] serviceModeDelay - The service mode timeout.
     * @param[in] meltdownDelay - The meltdown delay timeout.
     * @param[in] powerInterface - The object to use to do the power off
     * @param[in] func - A function to call right before the power
     *                   off occurs (after the delay).  May be
     *                   empty if no function is necessary.
     */
    EpowPowerOff(uint32_t serviceModeDelay, uint32_t meltdownDelay,
                 std::shared_ptr<PowerInterfaceBase> powerInterface,
                 PrePowerOffFunc func) :
        PowerOffAction("EPOW Power Off: " + std::to_string(serviceModeDelay) +
                           "s/" + std::to_string(meltdownDelay) + "s",
                       powerInterface, func),
        _serviceModeDelay(serviceModeDelay), _meltdownDelay(meltdownDelay),
        _serviceModeTimer(
            _event,
            std::bind(std::mem_fn(&EpowPowerOff::serviceModeTimerExpired),
                      this)),
        _meltdownTimer(
            _event,
            std::bind(std::mem_fn(&EpowPowerOff::meltdownTimerExpired), this))
    {}

    /**
     * @brief Starts the service mode timer.
     */
    void start() override
    {
        getLogger().log(
            fmt::format("Action {}: Starting service mode timer", name()));

        _serviceModeTimer.restartOnce(_serviceModeDelay);
    }

    /**
     * @brief Called when the service mode timer expires.
     *
     * Sets the thermal alert D-Bus property and starts the
     * meltdown timer.
     */
    void serviceModeTimerExpired()
    {
        getLogger().log(fmt::format(
            "Action {}: Service mode timer expired, starting meltdown timer",
            name()));

        _powerIface->thermalAlert(true);
        _meltdownTimer.restartOnce(_meltdownDelay);
    }

    /**
     * @brief Called when the meltdown timer expires.
     *
     * Executes a hard power off.
     */
    void meltdownTimerExpired()
    {
        getLogger().log(fmt::format(
            "Action {}: Meltdown timer expired, executing hard power off",
            name()));

        if (_prePowerOffFunc)
        {
            _prePowerOffFunc();
        }

        _powerIface->hardPowerOff();
        createBmcDump();
    }

    /**
     * @brief Attempts to cancel the action
     *
     * The service mode timer can be canceled.  The meltdown
     * timer cannot.
     *
     * @param[in] force - To force the cancel (like if the
     *                    system powers off).
     *
     * @return bool - If the cancel was successful
     */
    bool cancel(bool force) override
    {
        if (_serviceModeTimer.isEnabled())
        {
            _serviceModeTimer.setEnabled(false);
        }

        if (_meltdownTimer.isEnabled())
        {
            if (force)
            {
                _meltdownTimer.setEnabled(false);
            }
            else
            {
                getLogger().log("Cannot cancel running meltdown timer");
                return false;
            }
        }
        return true;
    }

  private:
    /**
     * @brief The number of seconds to wait until starting the uncancelable
     *        meltdown timer.
     */
    std::chrono::seconds _serviceModeDelay;

    /**
     * @brief The number of seconds to wait after the service mode
     *        timer expires before a hard power off will occur.
     */
    std::chrono::seconds _meltdownDelay;

    /**
     * @brief The service mode timer.
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>
        _serviceModeTimer;

    /**
     * @brief The meltdown timer.
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> _meltdownTimer;
};
} // namespace phosphor::fan::monitor
