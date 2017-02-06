#pragma once

#include <string>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>

namespace phosphor
{
namespace fan
{
namespace presence
{

class Rotor
{
    public:
        Rotor() = delete;
        Rotor(const Rotor&) = delete;
        Rotor(Rotor&&) = default;
        Rotor& operator=(const Rotor&) = delete;
        Rotor& operator=(Rotor&&) = default;
        ~Rotor() = default;

        Rotor(sdbusplus::bus::bus& bus,
              const std::string& match) :
              bus(bus),
              tachSignal(bus,
                         match.c_str(),
                         handleTachSignal)
        {
            //TODO Create inventory for this fan rotor
        }

    private:
        sdbusplus::bus::bus& bus;
        // Register tach signal callback handler
        sdbusplus::server::match::match tachSignal;

        static int handleTachSignal(sd_bus_message* msg,
                                    void* data,
                                    sd_bus_error* err);
};

} // namespace presence
} // namespace fan
} // namespace phosphor
