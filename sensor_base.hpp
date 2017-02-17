#pragma once


namespace phosphor
{
namespace fan
{
namespace presence
{

class FanEnclosure;
class Sensor
{
    public:
        Sensor() = delete;
        Sensor(const Sensor&) = delete;
        Sensor(Sensor&&) = default;
        Sensor& operator=(const Sensor&) = delete;
        Sensor& operator=(Sensor&&) = default;
        virtual ~Sensor() = default;

        virtual bool isPresent() = 0;

    protected:

};

} // namespace presence
} // namespace fan
} // namespace phosphor
