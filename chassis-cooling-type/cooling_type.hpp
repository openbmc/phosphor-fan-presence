#pragma once
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <libevdev/libevdev.h>

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
        using Property = std::string;
        using Value = sdbusplus::message::variant<bool, int64_t, std::string>;
        // Association between property and its value
        using PropertyMap = std::map<Property, Value>;
        using Interface = std::string;
        // Association between interface and the dbus property
        using InterfaceMap = std::map<Interface, PropertyMap>;
        using Object = sdbusplus::message::object_path;
        // Association between object and the interface
        using ObjectMap = std::map<Object, InterfaceMap>;

    public:
        CoolingType() = delete;
        CoolingType(CoolingType&&) = default;
        CoolingType& operator=(const CoolingType&) = delete;
        CoolingType& operator=(CoolingType&&) = delete;

        /**
         * @brief Constructs Cooling Type Object
         *
         * @param[in] bus - Dbus bus object
         */
        CoolingType(sdbusplus::bus::bus& bus) :
            bus(bus), gpioFd(0)
        {
            updateInventory();
        }

        ~CoolingType();

        // Set airCooled to true.
        void setAirCooled();
        // Set waterCooled to true.
        void setWaterCooled();
        ///! Update the inventory properties for cooling to the current values.
        void updateInventory();
        // Setup the GPIO device for reading cooling type.
        void setupGpio(std::string);

    private:
        /** @brief Connection for sdbusplus bus */
        sdbusplus::bus::bus& bus;
        struct libevdev* gpioDev = nullptr;
        // File descriptor for the GPIO file we are going to read.
        int gpioFd;
        const char* gpioPath;
        bool airCooled = false;
        bool waterCooled = false;

        /**
         * @brief Construct the inventory object map for CoolingType.
         *
         * @return The inventory object map to update inventory
         */
        ObjectMap getObjectMap(void);

};

}
}
}

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
