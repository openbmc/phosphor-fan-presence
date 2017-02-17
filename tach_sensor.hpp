#pragma once

#include <sdbusplus/bus.hpp>
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

        TachSensor(sdbusplus::bus::bus& bus,
                   const std::string& id,
                   FanEnclosure& fanEnc) : Sensor(id, fanEnc),
                       bus(bus)
        {
            // Nothing to do here
        }

        bool isPresent();

    private:
        sdbusplus::bus::bus& bus;
        int64_t tach = 0;

};

} // namespace presence
} // namespace fan
} // namespace phosphor
