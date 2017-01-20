#pragma once

namespace sdbusplus
{
namespace xyz
{
namespace openbmc_project
{
namespace Fan
{
namespace server
{

class Sensor
{
    public:
        Sensor() = delete;
        Sensor(const Sensor&) = delete;
        Sensor& operator=(const Sensor&) = delete;
        Sensor(Sensor&&) = delete;
        Sensor& operator=(Sensor&&) = delete;
        virtual ~Sensor() = default;

        /** @brief Constructor to put object onto bus at a dbus path.
         *  @param[in] bus - Bus to attach to.
         *  @param[in] path - Path to attach at.
         */
        Sensor(bus::bus& bus, const char* path);

    private:
        static constexpr auto _interface = "xyz.openbmc_project.Fan.Sensor";

};

} // namespace server
} // namespace Presence
} // namespace openbmc_project
} // namespace xyz
} // namespace sdbusplus
