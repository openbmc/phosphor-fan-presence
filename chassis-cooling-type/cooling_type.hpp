#pragma once

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

        /**
         * @brief Sets airCooled to true.
         */
        void setAirCooled();
        /**
         * @brief Sets waterCooled to true.
         */
        void setWaterCooled();
        /**
         * @brief Updates the inventory properties for CoolingType.
         */
        void updateInventory();
        /**
         * @brief Setup the GPIO device for reading cooling type.
         *
         * @param[in] std::string - Path to the GPIO device file to read
         */
        void setupGpio(std::string);

    private:
        // File descriptor for the GPIO file we are going to read.
        int gpioFd = 0;
        const char* gpioPath;
        bool airCooled = false;
        bool waterCooled = false;
};

}
}
}

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
