#pragma once
#include <vector>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include "fan.hpp"
#include "types.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{

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
class Zone
{
    public:

        Zone() = delete;
        Zone(const Zone&) = delete;
        Zone(Zone&&) = default;
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
         * @param[in] def - the fan zone definition data
         */
        Zone(Mode mode,
             sdbusplus::bus::bus& bus,
             const ZoneDefinition& def);

        /**
         * Sets all fans in the zone to the speed
         * passed in
         *
         * @param[in] speed - the fan speed
         */
        void setSpeed(uint64_t speed);

        /**
         * Sets the zone to full speed
         */
        inline void setFullSpeed()
        {
            if (_fullSpeed != 0)
            {
                setSpeed(_fullSpeed);
            }
        }

        /**
         * @brief Sets the automatic fan control allowed active state
         *
         * @param[in] group - A group that affects the active state
         * @param[in] isActiveAllow - Active state according to group
         */
        void setActiveAllow(const Group* group, bool isActiveAllow);

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
                    _properties[object][interface][property]);
        };

        /**
         * @brief Get the default floor speed
         *
         * @return - The defined default floor speed
         */
        inline auto getDefFloor()
        {
            return _defFloorSpeed;
        };

    private:

        /**
         * The dbus object
         */
        sdbusplus::bus::bus& _bus;

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
         * Automatic fan control active state
         */
        bool _isActive = true;

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
        std::map<const Group*, bool> _active;

        /**
         * @brief List of signal event arguments
         */
        std::vector<std::unique_ptr<EventData>> _signalEvents;

        /**
         * @brief list of Dbus matches for callbacks
         */
        std::vector<sdbusplus::server::match::match> _matches;

        /**
         * @brief Get a property value from the path/interface given
         *
         * @param[in] bus - the bus to use
         * @param[in] path - the dbus path name
         * @param[in] iface - the dbus interface name
         * @param[in] prop - the property name
         * @param[out] value - the value of the property
         */
        static void getProperty(sdbusplus::bus::bus& bus,
                                const std::string& path,
                                const std::string& iface,
                                const std::string& prop,
                                PropertyVariantType& value);

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
