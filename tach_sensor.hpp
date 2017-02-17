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

class TachSensor : public Sensor
{
    public:
        TachSensor() = delete;
        TachSensor(const TachSensor&) = delete;
        TachSensor(TachSensor&&) = delete;
        TachSensor& operator=(const TachSensor&) = delete;
        TachSensor& operator=(TachSensor&&) = delete;
        ~TachSensor() = default;

        TachSensor(
            sdbusplus::bus::bus& bus,
            const std::string& id,
            FanEnclosure& fanEnc) :
                Sensor(id, fanEnc),
                bus(bus),
                tachSignal(bus,
                           match(id).c_str(),
                           handleTachChangeSignal,
                           this)
        {
            // Nothing to do here
        }

        bool isPresent();

    private:
        sdbusplus::bus::bus& bus;
        sdbusplus::server::match::match tachSignal;
        int64_t tach = 0;

        static std::string match(std::string id)
        {
            return std::string("type='signal',"
                               "interface='org.freedesktop.DBus.Properties',"
                               "member='PropertiesChanged',"
                               "path='/xyz/openbmc_project/sensors/fan_tach/" +
                               id + "'");
        }
        // Tach signal callback handler
        static int handleTachChangeSignal(sd_bus_message* msg,
                                          void* data,
                                          sd_bus_error* err);

        void handleTachChange(sdbusplus::message::message& msg,
                              sd_bus_error* err);

};

} // namespace presence
} // namespace fan
} // namespace phosphor
