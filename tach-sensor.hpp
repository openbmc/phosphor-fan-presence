#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include "sensor-base.hpp"


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
        TachSensor(TachSensor&&) = default;
        TachSensor& operator=(const TachSensor&) = delete;
        TachSensor& operator=(TachSensor&&) = default;
        ~TachSensor() = default;

        TachSensor(sdbusplus::bus::bus& bus,
                   const std::string& id,
                   auto fanEnc) : Sensor(id, fanEnc),
                       bus(bus),
                       match("type='signal',"
                             "interface='org.freedesktop.DBus.Properties',"
                             "member='PropertiesChanged',"
                             "path='xyz/openbmc_project/sensors/fan_tach/" +
                                id + "'"),
                       tachSignal(bus,
                                  match.c_str(),
                                  handleTachChangeSignal,
                                  this)
        {
            tach = 0;
        }

        bool isPresent();

    private:
        sdbusplus::bus::bus& bus;
        const std::string match;
        sdbusplus::server::match::match tachSignal;
        int64_t tach;

        // Tach signal callback handler
        static int handleTachChangeSignal(sd_bus_message* msg,
                                          void* data,
                                          sd_bus_error* err);

        int handleTachChange(sd_bus_message* msg,
                             sd_bus_error* err);

};

} // namespace presence
} // namespace fan
} // namespace phosphor
