/**
 * Copyright © 2021 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "../manager.hpp"
#include "action.hpp"
#include "group.hpp"
#include "zone.hpp"

#include <nlohmann/json.hpp>

#include <chrono>
#include <functional>
#include <memory>
#include <vector>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

/**
 * @class TimerBasedActions - Action that wraps a list of actions with a timer
 *
 * Sets up a list of actions to be invoked when the defined timer expires.
 * Once for a `oneshot` timer or at each expiration of a `repeating` timer.
 */
class TimerBasedActions :
    public ActionBase,
    public ActionRegister<TimerBasedActions>
{
  public:
    /* Name of this action */
    static constexpr auto name = "call_actions_based_on_timer";

    TimerBasedActions() = delete;
    TimerBasedActions(const TimerBasedActions&) = delete;
    TimerBasedActions(TimerBasedActions&&) = delete;
    TimerBasedActions& operator=(const TimerBasedActions&) = delete;
    TimerBasedActions& operator=(TimerBasedActions&&) = delete;
    ~TimerBasedActions() = default;

    /**
     * @brief Call actions when timer expires
     *
     * @param[in] jsonObj - JSON configuration of this action
     * @param[in] groups - Groups of dbus objects the action uses
     */
    TimerBasedActions(const json& jsonObj, const std::vector<Group>& groups);

    /**
     * @brief Run the action
     *
     * Starts or stops a timer that runs a list of actions whenever the
     * timer expires. The configured timer is set to callback the list of
     * actions against the given zones and configured groups.
     *
     * Where any group does not have a configured value to be compared against,
     * the groups' service owned state is used to start/stop the timer. When any
     * service providing a group member is not owned, the timer is started and
     * if all members' services are owned, the timer is stopped.
     *
     * Where all groups have a configured value to compare against, that will be
     * compared against all members within each group to start/stop the timer.
     * When all group members have a given value and it matches what's in the
     * cache, the timer is started and if any do not match, the timer is
     * stopped.
     *
     * @param[in] zone - Zone to run the action on
     */
    void run(Zone& zone) override;

    /**
     * @brief Start the timer
     *
     * Starts the configured timer of this action if not already running
     */
    void startTimer();

    /**
     * @brief Stop the timer
     *
     * Stops the configured timer of this action if running
     */
    void stopTimer();

    /**
     * @brief Timer expire's callback
     *
     * Called each time the timer expires, running the configured actions
     */
    void timerExpired();

    /**
     * @brief Set the zones on the action and the timer's actions
     *
     * @param[in] zones - Zones for the action and timer's actions
     *
     * Sets the zones on this action and the timer's actions to run against
     */
    virtual void
        setZones(std::vector<std::reference_wrapper<Zone>>& zones) override;

  private:
    /* The timer for this action */
    Timer _timer;

    /* Whether timer triggered by groups' owner or property value states */
    bool _byOwner;

    /* Timer interval for this action's timer */
    std::chrono::microseconds _interval;

    /* Timer type for this action's timer */
    TimerType _type;

    /* List of actions to be called when the timer expires */
    std::vector<std::unique_ptr<ActionBase>> _actions;

    /**
     * @brief Parse and set the timer configuration
     *
     * @param[in] jsonObj - JSON object for the action
     *
     * Sets the timer configuration used to run the list of actions
     */
    void setTimerConf(const json& jsonObj);

    /**
     * @brief Parse and set the list of actions
     *
     * @param[in] jsonObj - JSON object for the action
     *
     * Sets the list of actions that is run when the timer expires
     */
    void setActions(const json& jsonObj);
};

} // namespace phosphor::fan::control::json
