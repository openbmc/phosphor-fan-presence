#pragma once

#include <sdbusplus/bus.hpp>
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
                       bus(bus)
        {
            tach = 0;
        }

        bool isPresent();

    private:
        sdbusplus::bus::bus& bus;
        int64_t tach;

};

} // namespace presence
} // namespace fan
} // namespace phosphor
