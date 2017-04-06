#pragma once
#include "utility.hpp"

namespace phosphor
{
namespace cooling
{
namespace type
{

constexpr auto INVENTORY_PATH = "/xyz/openbmc_project/inventory";
constexpr auto INVENTORY_INTF = "xyz.openbmc_project.Inventory.Manager";

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
         * @param[in] path - D-Bus path
         */
        void updateInventory(const std::string&);
        /**
         * @brief Setup the GPIO device for reading cooling type.
         *
         * @param[in] - Path to object to update
         */
        void setupGpio(const std::string&);

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
         * @param[in] - Path to object to update
         *
         * @return The inventory object map to update inventory
        */
        ObjectMap getObjectMap(const std::string&);

};

}
}
}

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
