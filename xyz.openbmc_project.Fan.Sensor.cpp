#include <sdbusplus/bus.hpp>
#include "xyz/openbmc_project/Fan/Sensor/server.hpp"

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

Sensor::Sensor(bus::bus& bus, const char* path)
        : _xyz_openbmc_project_Fan_Sensor_interface(
            bus, path, _interface, _vtable, this)
{
}

const vtable::vtable_t Sensor::_vtable[] = {
    vtable::start(),
    vtable::end()
};

} // namespace server
} // namespace Fan
} // namespace openbmc_project
} // namespace xyz
} // namespace sdbusplus
