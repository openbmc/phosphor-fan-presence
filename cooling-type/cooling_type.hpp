#pragma once
#include "utility.hpp"
#include <libevdev/libevdev.h>

namespace phosphor
{
namespace chassis
{
namespace cooling
{

constexpr auto INVENTORY_PATH = "/xyz/openbmc_project/inventory";
constexpr auto INVENTORY_INTF = "xyz.openbmc_project.Inventory.Manager";

struct FreeEvDev
{
    void operator()(struct libevdev* device) const
    {
        libevdev_free(device);
    }
};

class CoolingType
{
        using Property = std::string;
        using Value = sdbusplus::message::variant<bool>;
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
        CoolingType(sdbusplus::bus::bus& bus) : bus(bus)
        {
            //TODO: Issue openbmc/openbmc#1531 - means to default properties.
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
         *
         * @param[in] path - Path to object to update
         */
        void updateInventory(const std::string&);
        /**
         * @brief Setup and read the GPIO device for reading cooling type.
         *
         * @param[in] std::string - Path to the GPIO device file to read
         * @param[in] unsigned int - Event/key code to read (pin)
         */
        void readGpio(const std::string&, unsigned int);

    private:
        /** @brief Connection for sdbusplus bus */
        sdbusplus::bus::bus& bus;
        // File descriptor for the GPIO file we are going to read.
        phosphor::fan::util::FileDescriptor gpioFd = -1;
        bool airCooled = false;
        bool waterCooled = false;

        /**
         * @brief Construct the inventory object map for CoolingType.
         *
         * @param[in] path - Path to object to update
         *
         * @return The inventory object map to update inventory
         */
        ObjectMap getObjectMap(const std::string&);

};

}
}
}

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
