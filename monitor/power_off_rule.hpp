#pragma once

#include "logging.hpp"
#include "power_off_action.hpp"
#include "power_off_cause.hpp"

#include <memory>

namespace phosphor::fan::monitor
{

/**
 * @brief Describes when the rule should be checked.
 *        either right at PGOOD, or anytime at runtime.
 */
enum class PowerRuleState
{
    atPgood,
    runtime
};

/**
 * @class PowerOffRule
 *
 * This class implements a power off rule, which has a cause
 * that is based on fan status, and an action which is a type of
 * power off is the cause is satisfied.
 *
 * The user of this class calls the 'check()' method when fan
 * status may have changed, and then the power off action may
 * be started.
 */
class PowerOffRule
{
  public:
    PowerOffRule() = delete;
    ~PowerOffRule() = default;
    PowerOffRule(const PowerOffRule&) = delete;
    PowerOffRule& operator=(const PowerOffRule&) = delete;
    PowerOffRule(PowerOffRule&&) = delete;
    PowerOffRule& operator=(PowerOffRule&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] validState - What state the rule is valid for
     * @param[in] cause - The power off cause to use
     * @param[in] action - The power off action to use
     */
    PowerOffRule(PowerRuleState validState,
                 std::unique_ptr<PowerOffCause> cause,
                 std::unique_ptr<PowerOffAction> action) :
        _validState(validState),
        _cause(std::move(cause)), _action(std::move(action))
    {}

    /**
     * @brief Used to cancel a delay based power off when
     *        there is still time left.
     */
    void cancel()
    {
        _active = false;

        // force the cancel
        _action->cancel(true);
    }

    /**
     * @brief Checks the cause against the passed in fan health
     *        and starts the power off action if the cause
     *        is satisfied.
     */
    void check(PowerRuleState state, const FanHealth& fanHealth)
    {
        if (state == _validState)
        {
            auto satisfied = _cause->satisfied(fanHealth);

            if (!_active && satisfied)
            {
                // Start the action
                _active = true;
                _action->start();
                getLogger().log(fmt::format(
                    "Starting shutdown action '{}' due to cause '{}'",
                    _action->name(), _cause->name()));
            }
            else if (_active && !satisfied)
            {
                // Attempt to cancel the action, but don't force it
                if (_action->cancel(false))
                {
                    getLogger().log(fmt::format("Stopped shutdown action '{}'",
                                                _action->name()));
                    _active = false;
                }
                else
                {
                    getLogger().log(
                        fmt::format("Could not stop shutdown action '{}'",
                                    _action->name()));
                }
            }
        }
    }

    /**
     * @brief Says if there is an active power off in progress due to
     *        this rule.
     *
     * @return bool - If the rule is active or not
     */
    bool active() const
    {
        return _active;
    }

  private:
    /**
     * @brief The state the rule is valid for:
     *   atPgood - Only at the moment when PGOOD switches on.
     *   runtime - Anytime that power is on.
     */
    PowerRuleState _validState;

    /**
     * @brief If there is an active power off in progress.
     */
    bool _active{false};

    /**
     * @brief Base class pointer to the power off cause class
     */
    std::unique_ptr<PowerOffCause> _cause;

    /**
     * @brief Base class pointer to the power off action class
     */
    std::unique_ptr<PowerOffAction> _action;
};

} // namespace phosphor::fan::monitor
