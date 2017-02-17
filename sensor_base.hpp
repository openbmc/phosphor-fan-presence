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
        Sensor(Sensor&&) = delete;
        Sensor& operator=(const Sensor&) = delete;
        Sensor& operator=(Sensor&&) = delete;
        virtual ~Sensor() = default;

        Sensor(const std::string& id,
               FanEnclosure& fanEnc) :
            id(id),
            fanEnc(fanEnc)
        {
            //Nothing to do here
        }

        virtual bool isPresent() = 0;

    protected:
        const std::string id;
        FanEnclosure& fanEnc;

};

} // namespace presence
} // namespace fan
} // namespace phosphor
