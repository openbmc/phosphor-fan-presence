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
        ~Sensor() = default;

        Sensor(const std::string& id, auto fanEnc) :
            id(id),
            fanEnc(std::move(fanEnc))
        {
            //Nothing to do here
        }

        virtual bool isPresent() = 0;

    protected:
        const std::string id;
        std::shared_ptr<phosphor::fan::presence::FanEnclosure> fanEnc;

};

} // namespace presence
} // namespace fan
} // namespace phosphor
