#pragma once

#include <fmt/format.h>

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <chrono>
#include <deque>
#include <optional>
#include <utility>

namespace phosphor
{
namespace fan
{
namespace monitor
{

class Fan;

constexpr auto FAN_SENSOR_PATH = "/xyz/openbmc_project/sensors/fan_tach/";

/**
 * The mode fan monitor will run in:
 *   - init - only do the initialization steps
 *   - monitor - run normal monitoring algorithm
 */
enum class Mode
{
    init,
    monitor
};

/**
 * The mode that the timer is running in:
 *   - func - Transition to functional state timer
 *   - nonfunc - Transition to nonfunctional state timer
 */
enum class TimerMode
{
    func,
    nonfunc
};

/**
 * The mode that the method is running in:
 *   - time - Use a percentage based deviation
 *   - count - Run up/down count fault detection
 */
enum MethodMode
{
    timebased = 0,
    count
};

/**
 * @class TachSensor
 *
 * This class represents the sensor that reads a tach value.
 * It may also support a Target, which is the property used to
 * set a speed.  Since it doesn't necessarily have a Target, it
 * won't for sure know if it is running too slow, so it leaves
 * that determination to other code.
 *
 * This class has a parent Fan object that knows about all
 * sensors for that fan.
 */
class TachSensor
{
  public:
    TachSensor() = delete;
    TachSensor(const TachSensor&) = delete;
    // TachSensor is not moveable since the this pointer is used as systemd
    // callback context.
    TachSensor(TachSensor&&) = delete;
    TachSensor& operator=(const TachSensor&) = delete;
    TachSensor& operator=(TachSensor&&) = delete;
    ~TachSensor() = default;

    /**
     * @brief Constructor
     *
     * @param[in] mode - mode of fan monitor
     * @param[in] bus - the dbus object
     * @param[in] fan - the parent fan object
     * @param[in] id - the id of the sensor
     * @param[in] hasTarget - if the sensor supports
     *                        setting the speed
     * @param[in] funcDelay - Delay to mark functional
     * @param[in] interface - the interface of the target
     * @param[in] path - the object path of the sensor target
     * @param[in] factor - the factor of the sensor target
     * @param[in] offset - the offset of the sensor target
     * @param[in] method - the method of out of range
     * @param[in] threshold - the threshold of counter method
     * @param[in] ignoreAboveMax - whether to ignore being above max or not
     * @param[in] timeout - Normal timeout value to use
     * @param[in] errorDelay - Delay in seconds before creating an error
     *                         or std::nullopt if no errors.
     * @param[in] countInterval - In count mode interval
     *
     * @param[in] event - Event loop reference
     */
    TachSensor(Mode mode, sdbusplus::bus_t& bus, Fan& fan,
               const std::string& id, bool hasTarget, size_t funcDelay,
               const std::string& interface, const std::string& path,
               double factor, int64_t offset, size_t method, size_t threshold,
               bool ignoreAboveMax, size_t timeout,
               const std::optional<size_t>& errorDelay, size_t countInterval,
               const sdeventplus::Event& event);

    /**
     * @brief Reads a property from the input message and stores it in value.
     *        T is the value type.
     *
     *        Note: This can only be called once per message.
     *
     * @param[in] msg - the dbus message that contains the data
     * @param[in] interface - the interface the property is on
     * @param[in] propertName - the name of the property
     * @param[out] value - the value to store the property value in
     */
    template <typename T>
    static void readPropertyFromMessage(sdbusplus::message_t& msg,
                                        const std::string& interface,
                                        const std::string& propertyName,
                                        T& value)
    {
        std::string sensor;
        std::map<std::string, std::variant<T>> data;
        msg.read(sensor, data);

        if (sensor.compare(interface) == 0)
        {
            auto propertyMap = data.find(propertyName);
            if (propertyMap != data.end())
            {
                value = std::get<T>(propertyMap->second);
            }
        }
    }

    /**
     * @brief Returns the target speed value
     */
    uint64_t getTarget() const;

    /**
     * @brief Returns the input speed value
     */
    inline double getInput() const
    {
        return _tachInput;
    }

    /**
     * @brief Returns true if sensor has a target
     */
    inline bool hasTarget() const
    {
        return _hasTarget;
    }

    /**
     * @brief Returns the interface of the sensor target
     */
    inline std::string getInterface() const
    {
        return _interface;
    }

    /**
     * @brief Returns true if sensor has a D-Bus owner
     */
    inline bool hasOwner() const
    {
        return _hasOwner;
    }

    /**
     * @brief sets D-Bus owner status
     *
     * @param[in] val - new owner status
     */
    inline void setOwner(bool val)
    {
        _hasOwner = val;
    }

    /**
     * @brief Returns the factor of the sensor target
     */
    inline double getFactor() const
    {
        return _factor;
    }

    /**
     * @brief Returns a reference to the sensor's Fan object
     */
    inline Fan& getFan() const
    {
        return _fan;
    }

    /**
     * @brief Returns the offset of the sensor target
     */
    inline int64_t getOffset() const
    {
        return _offset;
    }

    /**
     * @brief Returns the method of out of range
     */
    inline size_t getMethod() const
    {
        return _method;
    }

    /**
     * @brief Returns the threshold of count method
     */
    inline size_t getThreshold() const
    {
        return _threshold;
    }

    /**
     * Set the sensor faulted counter
     */
    void setCounter(bool count);

    /**
     * @brief Returns the sensor faulted count
     */
    inline size_t getCounter() const
    {
        return _counter;
    }

    /**
     * Returns true if the hardware behind this
     * sensor is considered working OK/functional.
     */
    inline bool functional() const
    {
        return _functional;
    }

    /**
     * Set the functional status and update inventory to match
     *
     * @param[in] functional - The new state
     * @param[in] skipErrorTimer - If setting the sensor to
     *            nonfunctional, don't start the error timer.
     */
    void setFunctional(bool functional, bool skipErrorTimer = false);

    /**
     * @brief Says if the timer is running or not
     *
     * @return bool - if timer is currently running
     */
    inline bool timerRunning()
    {
        return _timer.isEnabled();
    }

    /**
     * @brief Stops the timer when the given mode differs and starts
     * the associated timer for the mode given if not already running
     *
     * @param[in] mode - mode of timer to start
     */
    void startTimer(TimerMode mode);

    /**
     * @brief Stops the timer
     */
    inline void stopTimer()
    {
        phosphor::logging::log<phosphor::logging::level::DEBUG>(
            fmt::format("Stop running timer on tach sensor {}.", _name)
                .c_str());
        _timer.setEnabled(false);
    }

    /**
     * @brief Says if the count timer is running
     *
     * @return bool - If count timer running
     */
    inline bool countTimerRunning() const
    {
        return _countTimer && _countTimer->isEnabled();
    }

    /**
     * @brief Stops the count timer
     */
    void stopCountTimer();

    /**
     * @brief Starts the count timer
     */
    void startCountTimer();

    /**
     * @brief Return the given timer mode's delay time
     *
     * @param[in] mode - mode of timer to get delay time for
     */
    std::chrono::microseconds getDelay(TimerMode mode);

    /**
     * Returns the sensor name
     */
    inline const std::string& name() const
    {
        return _name;
    };

    /**
     * @brief Says if the error timer is running
     *
     * @return bool - If the timer is running
     */
    bool errorTimerRunning() const
    {
        if (_errorTimer && _errorTimer->isEnabled())
        {
            return true;
        }
        return false;
    }

    /**
     * @brief Get the current allowed range of speeds
     *
     * @param[in] deviation - The configured deviation(in percent) allowed
     *
     * @return pair - Min/Max(optional) range of speeds allowed
     */
    std::pair<uint64_t, std::optional<uint64_t>>
        getRange(const size_t deviation) const;

    /**
     * @brief Processes the current state of the sensor
     */
    void processState();

    /**
     * @brief Resets the monitoring method of the sensor
     */
    void resetMethod();

    /**
     * @brief Refreshes the tach input and target values by
     *        reading them from D-Bus.
     */
    void updateTachAndTarget();

    /**
     * @brief return the previous tach values
     */
    const std::deque<uint64_t>& getPrevTach() const
    {
        return _prevTachs;
    }

    /**
     * @brief return the previous target values
     */
    const std::deque<uint64_t>& getPrevTarget() const
    {
        return _prevTargets;
    }

  private:
    /**
     * @brief Returns the match string to use for matching
     *        on a properties changed signal.
     */
    std::string getMatchString(const std::optional<std::string> path,
                               const std::string& interface);

    /**
     * @brief Reads the Target property and stores in _tachTarget.
     *        Also calls Fan::tachChanged().
     *
     * @param[in] msg - the dbus message
     */
    void handleTargetChange(sdbusplus::message_t& msg);

    /**
     * @brief Reads the Value property and stores in _tachInput.
     *        Also calls Fan::tachChanged().
     *
     * @param[in] msg - the dbus message
     */
    void handleTachChange(sdbusplus::message_t& msg);

    /**
     * @brief Updates the Functional property in the inventory
     *        for this tach sensor based on the value passed in.
     *
     * @param[in] functional - If the Functional property should
     *                         be set to true or false.
     */
    void updateInventory(bool functional);

    /**
     * @brief the dbus object
     */
    sdbusplus::bus_t& _bus;

    /**
     * @brief Reference to the parent Fan object
     */
    Fan& _fan;

    /**
     * @brief The name of the sensor, including the full path
     *
     * For example /xyz/openbmc_project/sensors/fan_tach/fan0
     */
    const std::string _name;

    /**
     * @brief The inventory name of the sensor, including the full path
     */
    const std::string _invName;

    /**
     * @brief If functional (not too slow).  The parent
     *        fan object sets this.
     */
    bool _functional = true;

    /**
     * @brief If the sensor has a Target property (can set speed)
     */
    const bool _hasTarget;

    /**
     * @brief If the sensor has a D-Bus owner
     */
    bool _hasOwner = true;

    /**
     * @brief Amount of time to delay updating to functional
     */
    const size_t _funcDelay;

    /**
     * @brief The interface that the target implements
     */
    const std::string _interface;

    /**
     * @brief The object path to set sensor's target
     */
    const std::string _path;

    /**
     * @brief The factor of target to get fan rpm
     */
    const double _factor;

    /**
     * @brief The offset of target to get fan rpm
     */
    const int64_t _offset;

    /**
     * @brief The method of out of range
     */
    const size_t _method;

    /**
     * @brief The threshold for count method
     */
    const size_t _threshold;

    /**
     * @brief Whether to ignore being above the max or not
     */
    const bool _ignoreAboveMax;

    /**
     * @brief The counter for count method
     */
    size_t _counter = 0;

    /**
     * @brief The input speed, from the Value dbus property
     */
    double _tachInput = 0;

    /**
     * @brief The current target speed, from the Target dbus property
     *        (if applicable)
     */
    uint64_t _tachTarget = 0;

    /**
     * @brief The timeout value to use
     */
    const size_t _timeout;

    /**
     * @brief Mode that current timer is in
     */
    TimerMode _timerMode;

    /**
     * The timer object
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> _timer;

    /**
     * @brief The match object for the Value properties changed signal
     */
    std::unique_ptr<sdbusplus::bus::match_t> tachSignal;

    /**
     * @brief The match object for the Target properties changed signal
     */
    std::unique_ptr<sdbusplus::bus::match_t> targetSignal;

    /**
     * @brief The number of seconds to wait between a sensor being set
     *        to nonfunctional and creating an error for it.
     *
     * If std::nullopt, no errors will be created.
     */
    const std::optional<size_t> _errorDelay;

    /**
     * @brief The timer that uses _errorDelay.  When it expires an error
     *        will be created for a faulted fan sensor (rotor).
     *
     * If _errorDelay is std::nullopt, then this won't be created.
     */
    std::unique_ptr<
        sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>>
        _errorTimer;

    /**
     * @brief The interval, in seconds, to use for the timer that runs
     *        the checks when using the 'count' method.
     */
    size_t _countInterval;

    /**
     * @brief The timer used by the 'count' method for determining
     *        functional status.
     */
    std::unique_ptr<
        sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>>
        _countTimer;

    /**
     * @brief record of previous targets
     */
    std::deque<uint64_t> _prevTargets;

    /**
     * @brief record of previous tach readings
     */
    std::deque<uint64_t> _prevTachs;
};

} // namespace monitor
} // namespace fan
} // namespace phosphor
