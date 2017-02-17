#include <vector>
#include <sdbusplus/bus.hpp>

int main(void)
{
    auto bus = sdbusplus::bus::new_default();

    while (true) //TODO Only necessary while powered on
    {
        // Respond to dbus signals
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
