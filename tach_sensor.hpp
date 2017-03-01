#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include "sensor_base.hpp"


namespace phosphor
{
namespace fan
{
namespace presence
{

/**
 * @class TachSensor
 * @brief OpenBMC Tach feedback sensor presence implementation
 * @details Derived sensor type that uses the tach feedback value to determine
 * the presence of the fan enclosure that contains this sensor
 */
class TachSensor : public Sensor
{
    public:
        TachSensor() = delete;
        TachSensor(const TachSensor&) = delete;
        TachSensor(TachSensor&&) = default;
        TachSensor& operator=(const TachSensor&) = delete;
        TachSensor& operator=(TachSensor&&) = default;
        ~TachSensor() = default;

        /**
         * @brief Constructs Tach Sensor Object
         *
         * @param[in] bus - Dbus bus object
         * @param[in] id - ID name of this sensor
         * @param[in] fanEnc - Reference to the fan enclosure with this sensor
         */
        TachSensor(sdbusplus::bus::bus& bus,
                   const std::string& id,
                   auto fanEnc) : Sensor(id, fanEnc),
                       bus(bus),
                       match("type='signal',"
                             "interface='org.freedesktop.DBus.Properties',"
                             "member='PropertiesChanged',"
                             "path='/xyz/openbmc_project/sensors/fan_tach/" +
                                id + "'"),
                       tachSignal(bus,
                                  match.c_str(),
                                  handleTachChangeSignal,
                                  this)
        {
            // Nothing to do here
        }

        /**
         * @brief Determine the presence of this sensor using the tach feedback
         *
         * @return Presence state based on tach feedback
         */
        bool isPresent();

    private:
        /** @brief Connection for sdbusplus bus */
        sdbusplus::bus::bus& bus;
        /** @brief Match string on the hwmon tach signal for this sensor ID */
        const std::string match;
        /** @brief Used to subscribe to dbus signals */
        sdbusplus::server::match::match tachSignal;
        /** @brief Tach speed value given from the signal */
        int64_t tach = 0;

        /**
         * @brief Callback function on tach change signals
         *
         * @param[out] msg - Data associated with the subscribed signal
         * @param[out] data - Pointer to this tach sensor object instance
         * @param[out] err - Contains any sdbus error reference if occurred
         *
         * @return 0
         */
        static int handleTachChangeSignal(sd_bus_message* msg,
                                          void* data,
                                          sd_bus_error* err);
        /**
         * @brief Determine & handle when the signal was a tach change
         *
         * @param[in] msg - Expanded sdbusplus message data
         * @param[in] err - Contains any sdbus error reference if occurred
         */
        void handleTachChange(sdbusplus::message::message& msg,
                              sd_bus_error* err);

};

} // namespace presence
} // namespace fan
} // namespace phosphor
