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
        TachSensor(TachSensor&&) = delete;
        TachSensor& operator=(const TachSensor&) = delete;
        TachSensor& operator=(TachSensor&&) = delete;
        ~TachSensor() = default;

        /**
         * @brief Constructs Tach Sensor Object
         *
         * @param[in] bus - Dbus bus object
         * @param[in] id - ID name of this sensor
         * @param[in] fanEnc - Reference to the fan enclosure with this sensor
         */
        TachSensor(
            sdbusplus::bus::bus& bus,
            const std::string match,
            std::shared_ptr<phosphor::fan::presence::FanEnclosure> fanEnc) :
                Sensor(id, fanEnc),
                bus(bus),
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
