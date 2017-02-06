#include <sdbusplus/bus.hpp>
#include "tach-handler.hpp"

namespace phosphor
{
namespace fan
{
namespace presence
{

    //Callback function
    int Rotor::handleTachSignal(sd_bus_message* msg,
                                void* usrData,
                                sd_bus_error* err)
    {
        //TODO Update inventory with fan presence when tach < 0
        return 0;
    }

} // namespace presence
} // namespace fan
} // namespace phosphor
