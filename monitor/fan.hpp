#pragma once

#include "config.h"

#include "tach_sensor.hpp"
#include "trust_manager.hpp"
#include "types.hpp"

#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

#include <tuple>
#include <vector>

namespace phosphor
{
namespace fan
{
namespace monitor
{

class System;

/**
 * @class Fan
 *
 * Represents a fan, which can contain 1 or more sensors which
 * loosely correspond to rotors.  See below.
 *
 * There is a sensor when hwmon exposes one, which means there is a
 * speed value to be read.  Sometimes there is a sensor per rotor,
 * and other times multiple rotors just use 1 sensor total where
 * the sensor reports the slowest speed of all of the rotors.
 *
 * A rotor's speed is set by writing the Target value of a sensor.
 * Sometimes each sensor in a fan supports having a Target, and other
 * times not all of them do.  A TachSensor object knows if it supports
 * the Target property.
 *
 * The strategy for monitoring fan speeds is as follows:
 *
 * Every time a Target (new speed written) or Input (actual speed read)
 * sensor changes, check if the input value is within some range of the target
 * value.  If it isn't, start a timer at the end of which the sensor will be
 * set to not functional.  If enough sensors in the fan are now nonfunctional,
 * set the whole fan to nonfunctional in the inventory.
 *
 * When sensor inputs come back within a specified range of the target,
 * stop its timer if running, make the sensor functional again if it wasn't,
 * and if enough sensors in the fan are now functional set the whole fan
 * back to functional in the inventory.
 */
class Fan
{
    using Property = std::string;
    using Value = std::variant<bool>;
    using PropertyMap = std::map<Property, Value>;

    using Interface = std::string;
    using InterfaceMap = std::map<Interface, PropertyMap>;

    using Object = sdbusplus::message::object_path;
    using ObjectMap = std::map<Object, InterfaceMap>;

  public:
    Fan() = delete;
    Fan(const Fan&) = delete;
    Fan(Fan&&) = default;
    Fan& operator=(const Fan&) = delete;
    Fan& operator=(Fan&&) = default;
    ~Fan() = default;

    /**
     * @brief Constructor
     *
     * @param mode - mode of fan monitor
     * @param bus - the dbus object
     * @param event - event loop reference
     * @param trust - the tach trust manager
     * @param def - the fan definition structure
     * @param system - Reference to the system object
     */
    Fan(Mode mode, sdbusplus::bus_t& bus, const sdeventplus::Event& event,
        std::unique_ptr<trust::Manager>& trust, const FanDefinition& def,
        System& system);

    /**
     * @brief Callback function for when an input sensor changes
     *
     * Starts a timer, where if it expires then the sensor
     * was out of range for too long and can be considered not functional.
     */
    void tachChanged(TachSensor& sensor);

    /**
     * @brief Calls tachChanged(sensor) on each sensor
     */
    void tachChanged();

    /**
     * @brief The callback function for the method
     *
     * Sets the sensor to not functional.
     * If enough sensors are now not functional,
     * updates the functional status of the whole
     * fan in the inventory.
     *
     * @param[in] sensor - the sensor for state update
     */
    void updateState(TachSensor& sensor);

    /**
     * @brief Get the name of the fan
     *
     * @return - The fan name
     */
    inline const std::string& getName() const
    {
        return _name;
    }

    /**
     * @brief Finds the target speed of this fan
     *
     * Finds the target speed from the list of sensors that make up this
     * fan. At least one sensor should contain a target speed value.
     *
     * @return - The target speed found from the list of sensors on the fan
     */
    uint64_t findTargetSpeed();

    /**
     * @brief Returns the contained TachSensor objects
     *
     * @return std::vector<std::shared_ptr<TachSensor>> - The sensors
     */
    const std::vector<std::shared_ptr<TachSensor>>& sensors() const
    {
        return _sensors;
    }

    /**
     * @brief Returns the presence status of the fan
     *
     * @return bool - If the fan is present or not
     */
    bool present() const
    {
        return _present;
    }

    /**
     * @brief Called from TachSensor when its error timer expires
     *        so an event log calling out the fan can be created.
     *
     * @param[in] sensor - The nonfunctional sensor
     */
    void sensorErrorTimerExpired(const TachSensor& sensor);

    /**
     * @brief Process the state of the given tach sensor without checking
     * any trust groups the sensor may be included in
     *
     * @param[in] sensor - Tach sensor to process
     *
     * This function is intended to check the current state of a tach sensor
     * regardless of whether or not the tach sensor is configured to be in any
     * trust groups.
     */
    void process(TachSensor& sensor);

    /**
     * @brief The function that runs when the power state changes
     *
     * @param[in] powerStateOn - If power is now on or not
     */
    void powerStateChanged(bool powerStateOn);

    /**
     * @brief Timer callback function that deals with sensors using
     *        the 'count' method for determining functional status.
     *
     * @param[in] sensor - TachSensor object
     */
    void countTimerExpired(TachSensor& sensor);

    /**
     * @brief Returns the number of tach sensors (Sensor.Value ifaces)
     *        on D-Bus at the last power on.
     */
    inline size_t numSensorsOnDBusAtPowerOn() const
    {
        return _numSensorsOnDBusAtPowerOn;
    }

    /**
     * @brief Returns true if the sensor input is not within
     * some deviation of the target.
     *
     * @param[in] sensor - the sensor to check
     */
    bool outOfRange(const TachSensor& sensor);

  private:
    /**
     * @brief Returns the number sensors that are nonfunctional
     */
    size_t countNonFunctionalSensors() const;

    /**
     * @brief Updates the Functional property in the inventory
     *        for the fan based on the value passed in.
     *
     * @param[in] functional - If the Functional property should
     *                         be set to true or false.
     *
     * @return - True if an exception was encountered during update
     */
    bool updateInventory(bool functional);

    /**
     * @brief Called by _monitorTimer to start fan monitoring some
     *        amount of time after startup.
     */
    void startMonitor();

    /**
     * @brief Called when the fan presence property changes on D-Bus
     *
     * @param[in] msg - The message from the propertiesChanged signal
     */
    void presenceChanged(sdbusplus::message_t& msg);

    /**
     * @brief Called when there is an interfacesAdded signal on the
     *        fan D-Bus path so the code can look for the 'Present'
     *        property value.
     *
     * @param[in] msg - The message from the interfacesAdded signal
     */
    void presenceIfaceAdded(sdbusplus::message_t& msg);

    /**
     * @brief the dbus object
     */
    sdbusplus::bus_t& _bus;

    /**
     * @brief The inventory name of the fan
     */
    const std::string _name;

    /**
     * @brief The percentage that the input speed must be below
     *        the target speed to be considered an error.
     *        Between 0 and 100.
     */
    const size_t _deviation;

    /**
     * The number of sensors that must be nonfunctional at the
     * same time in order for the fan to be set to nonfunctional
     * in the inventory.
     */
    const size_t _numSensorFailsForNonFunc;

    /**
     * The number of failed sensors
     */
    size_t _numFailedSensor = 0;

    /**
     * @brief The current functional state of the fan
     */
    bool _functional = true;

    /**
     * The sensor objects for the fan
     */
    std::vector<std::shared_ptr<TachSensor>> _sensors;

    /**
     * The tach trust manager object
     */
    std::unique_ptr<trust::Manager>& _trustManager;

#ifdef MONITOR_USE_JSON
    /**
     * @brief The number of seconds to wait after startup until
     *        fan sensors should checked against their targets.
     */
    size_t _monitorDelay;

    /**
     * @brief Expires after _monitorDelay to start fan monitoring.
     */
    sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic> _monitorTimer;
#endif

    /**
     * @brief Set to true when monitoring can start.
     */
    bool _monitorReady = false;

    /**
     * @brief Reference to the System object
     */
    System& _system;

    /**
     * @brief The match object for propertiesChanged signals
     *        for the inventory item interface to track the
     *        Present property.
     */
    sdbusplus::bus::match_t _presenceMatch;

    /**
     * @brief The match object for the interfacesAdded signal
     *        for the interface that has the Present property.
     */
    sdbusplus::bus::match_t _presenceIfaceAddedMatch;

    /**
     * @brief The current presence state
     */
    bool _present = false;

    /**
     * @brief The number of seconds to wait after a fan is removed before
     *        creating an event log for it.  If std::nullopt, then no
     *        event log will be created.
     */
    const std::optional<size_t> _fanMissingErrorDelay;

    /**
     * @brief The timer that uses the _fanMissingErrorDelay timeout,
     *        at the end of which an event log will be created.
     */
    std::unique_ptr<
        sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>>
        _fanMissingErrorTimer;

    /**
     * @brief If the fan and sensors should be set to functional when
     *        a fan plug is detected.
     */
    bool _setFuncOnPresent;

    /**
     * @brief The number of sensors that have their Sensor.Value interfaces
     *        on D-Bus at the last power on.
     *
     * Will be zero until the power turns on the first time.
     */
    size_t _numSensorsOnDBusAtPowerOn = 0;
};

} // namespace monitor
} // namespace fan
} // namespace phosphor
