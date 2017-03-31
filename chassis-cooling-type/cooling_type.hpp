#pragma once
#include "utility.hpp"

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
        ~CoolingType() = default;
        CoolingType(const CoolingType&) = delete;
        CoolingType(CoolingType&&) = default;
        CoolingType& operator=(const CoolingType&) = delete;
        CoolingType& operator=(CoolingType&&) = default;

        /**
         * @brief Constructs Cooling Type Object
         *
         * @param[in] bus - Dbus bus object
         */
        CoolingType(sdbusplus::bus::bus& bus)
        {
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
        void setupGpio(const std::string&);

    private:
        // File descriptor for the GPIO file we are going to read.
        phosphor::fan::util::FileDescriptor gpioFd = -1;
        bool airCooled = false;
        bool waterCooled = false;
};

}
}
}

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
