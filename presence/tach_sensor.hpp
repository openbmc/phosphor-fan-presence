#pragma once

#include <sdbusplus/server.hpp>
#include "sdbusplus.hpp"
#include "sensor_base.hpp"


namespace phosphor
{
namespace fan
{
namespace presence
{

using namespace sdbusplus::bus::match::rules;

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
         * @param[in] id - ID name of this sensor
         * @param[in] fanEnc - Reference to the fan enclosure with this sensor
         */
        TachSensor(
            const std::string& id,
            FanEnclosure& fanEnc,
            bool initialState = false) :
                Sensor(id, fanEnc),
                tachSignal(util::SDBusPlus::getBus(),
                           match(id).c_str(),
                           std::bind(
                               std::mem_fn(&TachSensor::handleTachChange),
                               this,
                               std::placeholders::_1)),
                state(initialState)
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
        /** @brief Used to subscribe to dbus signals */
        sdbusplus::server::match::match tachSignal;
        /** @brief Tach speed value given from the signal */
        bool state;

        /**
         * @brief Appends the fan sensor id to construct a match string
         *
         * @param[in] id - Fan sensor id
         *
         * @return Match string to register signal for the fan sensor id
         */
        static std::string match(const std::string& id)
        {
            return std::string(
                    interface("org.freedesktop.DBus.Properties") +
                    member("PropertiesChanged") +
                    type::signal() +
                    path("/xyz/openbmc_project/sensors/fan_tach/" + id) +
                    argN(0, "xyz.openbmc_project.Sensor.Value"));
        }
        /**
         * @brief Handle when the signal was a tach change
         *
         * @param[in] msg - Expanded sdbusplus message data
         */
        void handleTachChange(sdbusplus::message::message& msg);

};

} // namespace presence
} // namespace fan
} // namespace phosphor
