#pragma once
#include <algorithm>
#include <cassert>
#include <chrono>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <vector>
#include "fan.hpp"
#include "types.hpp"
#include "xyz/openbmc_project/Control/ThermalMode/server.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{

using ThermalObject = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Control::server::ThermalMode>;

/**
 * The mode fan control will run in:
 *   - init - only do the initialization steps
 *   - control - run normal control algorithms
 */
enum class Mode
{
    init,
    control
};

/**
 * @class Represents a fan control zone, which is a group of fans
 * that behave the same.
 */
class Zone : public ThermalObject
{
    public:

        Zone() = delete;
        Zone(const Zone&) = delete;
        Zone(Zone&&) = delete;
        Zone& operator=(const Zone&) = delete;
        Zone& operator=(Zone&&) = delete;
        ~Zone() = default;

        /**
         * Constructor
         * Creates the appropriate fan objects based on
         * the zone definition data passed in.
         *
         * @param[in] mode - mode of fan control
         * @param[in] bus - the dbus object
         * @param[in] path - object instance path
         * @param[in] event - Event loop reference
         * @param[in] def - the fan zone definition data
         */
        Zone(Mode mode,
             sdbusplus::bus::bus& bus,
             const std::string& path,
             const sdeventplus::Event& event,
             const ZoneDefinition& def);

        /**
         * Sets all fans in the zone to the speed
         * passed in when the zone is active
         *
         * @param[in] speed - the fan speed
         */
        void setSpeed(uint64_t speed);

        /**
         * Sets the zone to full speed regardless of zone's active state
         */
        void setFullSpeed();

        /**
         * @brief Sets the automatic fan control allowed active state
         *
         * @param[in] group - A group that affects the active state
         * @param[in] isActiveAllow - Active state according to group
         */
        void setActiveAllow(const Group* group, bool isActiveAllow);

        /**
         * @brief Sets the floor change allowed state
         *
         * @param[in] group - A group that affects floor changes
         * @param[in] isAllow - Allow state according to group
         */
        inline void setFloorChangeAllow(const Group* group, bool isAllow)
        {
            _floorChange[*(group)] = isAllow;
        }

        /**
         * @brief Sets the decrease allowed state of a group
         *
         * @param[in] group - A group that affects speed decreases
         * @param[in] isAllow - Allow state according to group
         */
        inline void setDecreaseAllow(const Group* group, bool isAllow)
        {
            _decAllowed[*(group)] = isAllow;
        }

        /**
         * @brief Sets a given object's property value
         *
         * @param[in] object - Name of the object containing the property
         * @param[in] interface - Interface name containing the property
         * @param[in] property - Property name
         * @param[in] value - Property value
         */
        template <typename T>
        void setPropertyValue(const char* object,
                              const char* interface,
                              const char* property,
                              T value)
        {
            _properties[object][interface][property] = value;
        };

        /**
         * @brief Get the value of an object's property
         *
         * @param[in] object - Name of the object containing the property
         * @param[in] interface - Interface name containing the property
         * @param[in] property - Property name
         *
         * @return - The property value
         */
        template <typename T>
        inline auto getPropertyValue(const std::string& object,
                                     const std::string& interface,
                                     const std::string& property)
        {
            return sdbusplus::message::variant_ns::get<T>(
                    _properties.at(object).at(interface).at(property));
        };

        /**
         * @brief Get the object's property variant
         *
         * @param[in] object - Name of the object containing the property
         * @param[in] interface - Interface name containing the property
         * @param[in] property - Property name
         *
         * @return - The property variant
         */
        inline auto getPropValueVariant(const std::string& object,
                                        const std::string& interface,
                                        const std::string& property)
        {
            return _properties.at(object).at(interface).at(property);
        };

        /**
         * @brief Remove an object's interface
         *
         * @param[in] object - Name of the object with the interface
         * @param[in] interface - Interface name to remove
         */
        inline void removeObjectInterface(const char* object,
                                          const char* interface)
        {
            auto it = _properties.find(object);
            if (it != std::end(_properties))
            {
                _properties[object].erase(interface);
            }
        }

        /**
         * @brief Remove a service associated to a group
         *
         * @param[in] group - Group associated with service
         * @param[in] name - Service name to remove
         */
        void removeService(const Group* group,
                           const std::string& name);

        /**
         * @brief Set or update a service name owner in use
         *
         * @param[in] group - Group associated with service
         * @param[in] name - Service name
         * @param[in] hasOwner - Whether the service is owned or not
         */
        void setServiceOwner(const Group* group,
                             const std::string& name,
                             const bool hasOwner);

        /**
         * @brief Set or update all services for a group
         *
         * @param[in] group - Group to get service names for
         */
        void setServices(const Group* group);

        /**
         * @brief Get the group's list of service names
         *
         * @param[in] group - Group to get service names for
         *
         * @return - The list of service names
         */
        inline auto getGroupServices(const Group* group)
        {
            return _services.at(*group);
        }

        /**
         * @brief Initialize a set speed event properties and actions
         *
         * @param[in] event - Set speed event
         */
        void initEvent(const SetSpeedEvent& event);

        /**
         * @brief Removes all the set speed event properties and actions
         *
         * @param[in] event - Set speed event
         */
        void removeEvent(const SetSpeedEvent& event);

        /**
         * @brief Get the default floor speed
         *
         * @return - The defined default floor speed
         */
        inline auto getDefFloor()
        {
            return _defFloorSpeed;
        };

        /**
         * @brief Get the ceiling speed
         *
         * @return - The current ceiling speed
         */
        inline auto& getCeiling() const
        {
            return _ceilingSpeed;
        };

        /**
         * @brief Set the ceiling speed to the given speed
         *
         * @param[in] speed - Speed to set the ceiling to
         */
        inline void setCeiling(uint64_t speed)
        {
            _ceilingSpeed = speed;
        };

        /**
         * @brief Swaps the ceiling key value with what's given and
         * returns the value that was swapped.
         *
         * @param[in] keyValue - New ceiling key value
         *
         * @return - Ceiling key value prior to swapping
         */
        inline auto swapCeilingKeyValue(int64_t keyValue)
        {
            std::swap(_ceilingKeyValue, keyValue);
            return keyValue;
        };

        /**
         * @brief Get the increase speed delta
         *
         * @return - The current increase speed delta
         */
        inline auto& getIncSpeedDelta() const
        {
            return _incSpeedDelta;
        };

        /**
         * @brief Get the decrease speed delta
         *
         * @return - The current decrease speed delta
         */
        inline auto& getDecSpeedDelta() const
        {
            return _decSpeedDelta;
        };

        /**
         * @brief Set the floor speed to the given speed and increase target
         * speed to the floor when target is below floor where floor changes
         * are allowed.
         *
         * @param[in] speed - Speed to set the floor to
         */
        void setFloor(uint64_t speed);

        /**
         * @brief Set the requested speed base to be used as the speed to
         * base a new requested speed target from
         *
         * @param[in] speedBase - Base speed value to use
         */
        inline void setRequestSpeedBase(uint64_t speedBase)
        {
            _requestSpeedBase = speedBase;
        };

        /**
         * @brief Calculate the requested target speed from the given delta
         * and increase the fan speeds, not going above the ceiling.
         *
         * @param[in] targetDelta - The delta to increase the target speed by
         */
        void requestSpeedIncrease(uint64_t targetDelta);

        /**
         * @brief Calculate the requested target speed from the given delta
         * and increase the fan speeds, not going above the ceiling.
         *
         * @param[in] targetDelta - The delta to increase the target speed by
         */
        void requestSpeedDecrease(uint64_t targetDelta);

        /**
         * @brief Callback function for the increase timer that delays
         * processing of requested speed increases while fans are increasing
         */
        void incTimerExpired();

        /**
         * @brief Callback function for the decrease timer that processes any
         * requested speed decreases if allowed
         */
        void decTimerExpired();

        /**
         * @brief Get the event loop used with this zone's timers
         *
         * @return - The event loop for timers
         */
        inline auto& getEventLoop()
        {
            return _eventLoop;
        }

        /**
         * @brief Get the list of signal events
         *
         * @return - List of signal events
         */
        inline auto& getSignalEvents()
        {
            return _signalEvents;
        }

        /**
         * @brief Find the first instance of a signal event
         *
         * @param[in] signal - Event signal to find
         * @param[in] eGroup - Group associated with the signal
         * @param[in] eActions - List of actions associated with the signal
         *
         * @return - Iterator to the stored signal event
         */
        std::vector<SignalEvent>::iterator findSignal(
            const Signal& signal,
            const Group& eGroup,
            const std::vector<Action>& eActions);

        /**
         * @brief Remove the given signal event
         *
         * @param[in] seIter - Iterator pointing to the signal event to remove
         */
        inline void removeSignal(std::vector<SignalEvent>::iterator& seIter)
        {
            assert(seIter != std::end(_signalEvents));
            std::get<signalEventDataPos>(*seIter).reset();
            if (std::get<signalMatchPos>(*seIter) != nullptr)
            {
                std::get<signalMatchPos>(*seIter).reset();
            }
            _signalEvents.erase(seIter);
        }

        /**
         * @brief Get the list of timer events
         *
         * @return - List of timer events
         */
        inline auto& getTimerEvents()
        {
            return _timerEvents;
        }

        /**
         * @brief Find the first instance of a timer event
         *
         * @param[in] eventGroup - Group associated with a timer
         * @param[in] eventActions - List of actions associated with a timer
         *
         * @return - Iterator to the timer event
         */
        std::vector<TimerEvent>::iterator findTimer(
                const Group& eventGroup,
                const std::vector<Action>& eventActions);

        /**
         * @brief Add a timer to the list of timer based events
         *
         * @param[in] group - Group associated with a timer
         * @param[in] actions - List of actions associated with a timer
         * @param[in] tConf - Configuration for the new timer
         */
        void addTimer(const Group& group,
                      const std::vector<Action>& actions,
                      const TimerConf& tConf);

        /**
         * @brief Remove the given timer event
         *
         * @param[in] teIter - Iterator pointing to the timer event to remove
         */
        inline void removeTimer(std::vector<TimerEvent>::iterator& teIter)
        {
            _timerEvents.erase(teIter);
        }

        /**
         * @brief Callback function for event timers that processes the given
         * actions for a group
         *
         * @param[in] eventGroup - Group to process actions on
         * @param[in] eventActions - List of event actions to run
         */
        void timerExpired(const Group& eventGroup,
                          const std::vector<Action>& eventActions);

        /**
         * @brief Get the service for a given path and interface from cached
         * dataset and add a service that's not found
         *
         * @param[in] path - Path to get service for
         * @param[in] intf - Interface to get service for
         *
         * @return - The service name
         */
        const std::string& getService(const std::string& path,
                                      const std::string& intf);

        /**
         * @brief Add a set of services for a path and interface
         * by retrieving all the path subtrees to the given depth
         * from root for the interface
         *
         * @param[in] path - Path to add services for
         * @param[in] intf - Interface to add services for
         * @param[in] depth - Depth of tree traversal from root path
         *
         * @return - The associated service to the given path and interface
         * or empty string for no service found
         */
        const std::string& addServices(const std::string& path,
                                       const std::string& intf,
                                       int32_t depth);

    private:

        /**
         * The dbus object
         */
        sdbusplus::bus::bus& _bus;

        /**
         * Zone object path
         */
        const std::string _path;

        /**
         * Full speed for the zone
         */
        const uint64_t _fullSpeed;

        /**
         * The zone number
         */
        const size_t _zoneNum;

        /**
         * The default floor speed for the zone
         */
        const uint64_t _defFloorSpeed;

        /**
         * The default ceiling speed for the zone
         */
        const uint64_t _defCeilingSpeed;

        /**
         * The floor speed to not go below
         */
        uint64_t _floorSpeed = _defFloorSpeed;

        /**
         * The ceiling speed to not go above
         */
        uint64_t _ceilingSpeed = _defCeilingSpeed;

        /**
         * The previous sensor value for calculating the ceiling
         */
        int64_t _ceilingKeyValue = 0;

        /**
         * Automatic fan control active state
         */
        bool _isActive = true;

        /**
         * Target speed for this zone
         */
        uint64_t _targetSpeed = _fullSpeed;

        /**
         * Speed increase delta
         */
        uint64_t _incSpeedDelta = 0;

        /**
         * Speed decrease delta
         */
        uint64_t _decSpeedDelta = 0;

        /**
         * Requested speed base
         */
        uint64_t _requestSpeedBase = 0;

        /**
         * Speed increase delay in seconds
         */
        std::chrono::seconds _incDelay;

        /**
         * Speed decrease interval in seconds
         */
        std::chrono::seconds _decInterval;

        /**
         * The increase timer object
         */
        Timer _incTimer;

        /**
         * The decrease timer object
         */
        Timer _decTimer;

        /**
         * Event loop used on set speed event timers
         */
        sdeventplus::Event _eventLoop;

        /**
         * The vector of fans in this zone
         */
        std::vector<std::unique_ptr<Fan>> _fans;

        /**
         * @brief Map of object property values
         */
        std::map<std::string,
                 std::map<std::string,
                          std::map<std::string,
                                   PropertyVariantType>>> _properties;

        /**
         * @brief Map of active fan control allowed by groups
         */
        std::map<const Group, bool> _active;

        /**
         * @brief Map of floor change allowed by groups
         */
        std::map<const Group, bool> _floorChange;

        /**
         * @brief Map of groups controlling decreases allowed
         */
        std::map<const Group, bool> _decAllowed;

        /**
         * @brief Map of group service names
         */
        std::map<const Group, std::vector<Service>> _services;

        /**
         * @brief Map tree of paths to services of interfaces
         */
        std::map<std::string,
                std::map<std::string,
                std::vector<std::string>>> _servTree;

        /**
         * @brief List of signal event arguments and Dbus matches for callbacks
         */
        std::vector<SignalEvent> _signalEvents;

        /**
         * @brief List of timers for events
         */
        std::vector<TimerEvent> _timerEvents;

        /**
         * @brief Save the thermal control custom mode property
         * to persisted storage
         */
        void saveCustomMode();

        /**
         * @brief Get the request speed base if defined, otherwise the
         * the current target speed is returned
         *
         * @return - The request speed base or current target speed
         */
        inline auto getRequestSpeedBase() const
        {
            return (_requestSpeedBase != 0) ? _requestSpeedBase : _targetSpeed;
        };

        /**
         * @brief Dbus signal change callback handler
         *
         * @param[in] msg - Expanded sdbusplus message data
         * @param[in] eventData - The single event's data
         */
        void handleEvent(sdbusplus::message::message& msg,
                         const EventData* eventData);
};

}
}
}
