#pragma once

#include <sdbusplus/bus.hpp>
#include "fan_properties.hpp"
#include "sensor_base.hpp"


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

namespace phosphor
{
namespace fan
{
namespace presence
{

/**
 * @brief Specifies the defined presence states of a fan enclosure
 */
typedef enum presenceState
{
    UNKNOWN,
    PRESENT,
    NOT_PRESENT
} presenceState;

/**
 * @class FanEnclosure
 * @brief OpenBMC fan enclosure inventory presence implementation
 * @details Inventory is based on the fan enclosure being present or not. This
 * class represents that fan enclosure and updates its presences status within
 * its inventory object based on the status of all its sensors.
 */
class FanEnclosure
{
    public:
        FanEnclosure() = delete;
        FanEnclosure(const FanEnclosure&) = delete;
        FanEnclosure(FanEnclosure&&) = default;
        FanEnclosure& operator=(const FanEnclosure&) = delete;
        FanEnclosure& operator=(FanEnclosure&&) = delete;
        ~FanEnclosure() = default;

        /**
         * @brief Constructs Fan Enclosure Object
         *
         * @param[in] bus - Dbus bus object
         * @param[in] fanProp - Fan enclosure properties
         */
        FanEnclosure(sdbusplus::bus::bus& bus,
                     const phosphor::fan::Properties& fanProp) :
                        bus(bus),
                        invPath(std::get<0>(fanProp)),
                        fanDesc(std::get<1>(fanProp))
        {
            //Add this fan to inventory
            updInventory();
        }

        /**
         * @brief Update inventory when the determined presence of this fan
         * enclosure has changed
         */
        void updInventory();
        /**
         * @brief Add a sensor association to this fan enclosure
         *
         * @param[in] sensor - Sensor associated to this fan enclosure
         */
        void addSensor(
            std::unique_ptr<Sensor>&& sensor);

    private:
        /** @brief Connection for sdbusplus bus */
        sdbusplus::bus::bus& bus;
        /** @brief Inventory path for this fan enclosure */
        const std::string invPath;
        /** @brief Description used as 'PrettyName' on inventory object */
        const std::string fanDesc;
        /** @brief List of sensors associated with this fan enclosure */
        std::vector<std::unique_ptr<Sensor>> sensors;
        /** @brief Last known presence state of this fan enclosure */
        presenceState presState = UNKNOWN;

        /**
         * @brief Get the current presence state based on all sensors
         *
         * @return Current presence state determined from all sensors
         */
        presenceState getCurPresState();
        //TODO Move getInvService() to a utility type file
        /**
         * @brief Get the inventory service name from the mapper object
         *
         * @return The inventory manager service name
         */
        std::string getInvService();
        /**
         * @brief Construct the inventory object map
         *
         * @param[in] Current presence state
         *
         * @return The inventory object map to update inventory
         */
        ObjectMap getObjectMap(const bool curPresState);

};

} // namespace presence
} // namespace fan
} // namespace phosphor
