#pragma once

#include "fan_properties.hpp"
#include "sdbusplus.hpp"
#include "sensor_base.hpp"


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
    NOT_PRESENT,
    PRESENT,
    UNKNOWN
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
        FanEnclosure() = delete;
        FanEnclosure(const FanEnclosure&) = delete;
        FanEnclosure(FanEnclosure&&) = default;
        FanEnclosure& operator=(const FanEnclosure&) = delete;
        FanEnclosure& operator=(FanEnclosure&&) = delete;
        ~FanEnclosure() = default;

        /**
         * @brief Constructs Fan Enclosure Object
         *
         * @param[in] fanProp - Fan enclosure properties
         * @param[in] initialState - The initial state of the enclosure.
         */
        explicit FanEnclosure(const phosphor::fan::Properties& fanProp,
                              presenceState initialState = UNKNOWN) :
                        invPath(std::get<0>(fanProp)),
                        fanDesc(std::get<1>(fanProp)),
                        presState(initialState)
        {
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
        /** @brief Inventory path for this fan enclosure */
        const std::string invPath;
        /** @brief Description used as 'PrettyName' on inventory object */
        const std::string fanDesc;
        /** @brief List of sensors associated with this fan enclosure */
        std::vector<std::unique_ptr<Sensor>> sensors;
        /** @brief Last known presence state of this fan enclosure */
        presenceState presState;

        /**
         * @brief Get the current presence state based on all sensors
         *
         * @return Current presence state determined from all sensors
         */
        presenceState getCurPresState();

        /**
         * @brief Construct the inventory object map
         *
         * @param[in] Current presence state
         *
         * @return The inventory object map to update inventory
         */
        ObjectMap getObjectMap(bool curPresState);

};

} // namespace presence
} // namespace fan
} // namespace phosphor
