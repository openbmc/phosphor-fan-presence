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
         * @param[in] bus - the dbus object
         * @param[in] def - the fan zone definition data
         */
        Zone(sdbusplus::bus::bus& bus,
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
         * @brief Sets a given object's property value
         *
         * @param[in] object - Name of the object containing the property
         * @param[in] property - Property name
         * @param[in] value - Property value
         */
        void setPropertyValue(const char* object,
                              const char* property,
                              bool value)
        {
            _properties[object][property] = value;
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
         * The vector of fans in this zone
         */
        std::vector<std::unique_ptr<Fan>> _fans;

        /**
         * @brief Map of object property values
         */
        std::map<std::string, std::map<std::string, bool>> _properties;

        /**
         * @brief List of signal event arguments
         */
        std::vector<std::unique_ptr<SignalEvent>> _signalEvents;

        /**
         * @brief list of Dbus matches for callbacks
         */
        std::vector<sdbusplus::server::match::match> _matches;

        /**
         * @brief Dbus signal change handler
         *
         * @param[in] msg - Data associated with the subscribed signal
         * @param[in] data - Pointer to the event sensor's data
         * @param[in] err - Contains any sdbus error reference if occurred
         */
        static int signalHandler(sd_bus_message* msg,
                                 void* data,
                                 sd_bus_error* err);

         /**
          * @brief Envokes the assigned handler and action
          *
          * @param[in] msg - Expanded sdbusplus message data
          * @param[in] eventData - The event's data
          */
         void handleEvent(sdbusplus::message::message& msg,
                          const Handler& handler);
};

}
}
}
