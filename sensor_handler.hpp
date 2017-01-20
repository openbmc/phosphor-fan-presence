#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include "xyz/openbmc_project/Fan/Sensor/server.hpp"

namespace phosphor
{
namespace fan
{
    class Sensor : public sdbusplus::server::object::object<
                sdbusplus::xyz::openbmc_project::Fan::server::Sensor>
    {
    public:
        Sensor() = delete;
        Sensor(const Sensor&) = delete;
        Sensor& operator=(const Sensor&) = delete;
        Sensor(Sensor&&) = delete;
        Sensor& operator=(Sensor&&) = delete;
        ~Sensor() = default;

        Sensor(sdbusplus::bus::bus& bus,
            const std::string& objPath) :
            sdbusplus::server::object::object<
                sdbusplus::xyz::openbmc_project::Fan::server::Sensor>(
                    bus, objPath.c_str()),
                path(objPath)
        {

        }

    private:
        std::string path;
    };
} // namespace fan
} // namespace phosphor
