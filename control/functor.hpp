#pragma once

#include "types.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{
class Zone;

/**
 * @brief Create a handler function object
 *
 * @param[in] handler - The handler being created
 *
 * @return - The created handler function object
 */
template <typename T>
auto make_handler(T&& handler)
{
    return Handler(std::forward<T>(handler));
}

/**
 * @struct Property Changed
 * @brief A match filter functor for Dbus property value changed signals
 *
 * @tparam T - The type of the property value
 * @tparam U - The type of the handler
 */
template <typename T, typename U>
struct PropertyChanged
{
    PropertyChanged() = delete;
    ~PropertyChanged() = default;
    PropertyChanged(const PropertyChanged&) = default;
    PropertyChanged& operator=(const PropertyChanged&) = default;
    PropertyChanged(PropertyChanged&&) = default;
    PropertyChanged& operator=(PropertyChanged&&) = default;
    PropertyChanged(const char* iface,
                    const char* property,
                    U&& handler) :
        _iface(iface),
        _property(property),
        _handler(std::forward<U>(handler)) { }

    /** @brief Run signal handler function
     *
     * Extract the property from the PropertiesChanged
     * message and run the handler function.
     */
    void operator()(sdbusplus::bus::bus&,
                    sdbusplus::message::message& msg,
                    Zone& zone) const
    {
        std::map<std::string, sdbusplus::message::variant<T>> properties;
        const char* iface = nullptr;

        msg.read(iface);
        if (!iface || strcmp(iface, _iface))
        {
            return;
        }

        msg.read(properties);
        auto it = properties.find(_property);
        if (it == properties.cend())
        {
            return;
        }

        _handler(zone, std::forward<T>(it->second.template get<T>()));
    }

private:
    const char* _iface;
    const char* _property;
    U _handler;
};

/**
 * @brief Used to process a Dbus property changed signal event
 *
 * @param[in] iface - Sensor value interface
 * @param[in] property - Sensor value property
 * @param[in] handler - Handler function to perform
 *
 * @tparam T - The type of the property
 * @tparam U - The type of the handler
 */
template <typename T, typename U>
auto propertySignal(const char* iface,
                    const char* property,
                    U&& handler)
{
    return PropertyChanged<T, U>(iface, property, std::move(handler));
}

} // namespace control
} // namespace fan
} // namespace phosphor
