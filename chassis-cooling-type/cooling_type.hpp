#pragma once
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

namespace phosphor
{
namespace chassis
{
namespace cooling
{
constexpr auto CHASSIS_PATH = "/xyz/openbmc_project/inventory/system/chassis";
constexpr auto CHASSIS_BUSNAME = "xyz.openbmc_project.Inventory.System.Chassis";

class CoolingType
{
    public:
        CoolingType() = delete;
        ~CoolingType();

        /**
         * @brief Constructs Cooling Type Object
         *
         * @param[in] bus - Dbus bus object
         */
        CoolingType(sdbusplus::bus::bus& bus) : 
                    gpioFd(0)
        {
            updateInventory();
        }

        // Set airCooled to true.
        void setAirCooled();
        // Set waterCooled to true.
        void setWaterCooled();
        // Update the inventory properties for cooling to the current values.
        void updateInventory();
        // Setup the GPIO device for reading cooling type.
        void setupGpio(std::string);

    private:
        // File descriptor for the GPIO file we are going to read.
        int gpioFd;
        const char* gpioPath;
        bool airCooled = false;
        bool waterCooled = false;
};

}
}
}
