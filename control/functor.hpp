#pragma once

#include "types.hpp"
#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace fan
{
namespace control
{
class Zone;

using namespace phosphor::logging;

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
 * @brief Create an action function object
 *
 * @param[in] action - The action being created
 *
 * @return - The created action function object
 */
template <typename T>
auto make_action(T&& action)
{
    return Action(std::forward<T>(action));
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
            log<level::ERR>("Unable to find property on interface",
                            entry("PROPERTY=%s", _property),
                            entry("INTERFACE=%s", _iface));
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
    return PropertyChanged<T, U>(iface, property, std::forward<U>(handler));
}

/**
 * @struct Interface Added
 * @brief A match filter functor for Dbus interface added signals
 *
 * @tparam T - The type of the property value
 * @tparam U - The type of the handler
 */
template <typename T, typename U>
struct InterfaceAdded
{
    InterfaceAdded() = delete;
    ~InterfaceAdded() = default;
    InterfaceAdded(const InterfaceAdded&) = default;
    InterfaceAdded& operator=(const InterfaceAdded&) = default;
    InterfaceAdded(InterfaceAdded&&) = default;
    InterfaceAdded& operator=(InterfaceAdded&&) = default;
    InterfaceAdded(const char* path,
                   const char* iface,
                   const char* property,
                   U&& handler) :
        _path(path),
        _iface(iface),
        _property(property),
        _handler(std::forward<U>(handler)) { }

    /** @brief Run signal handler function
     *
     * Extract the property from the InterfacesAdded
     * message and run the handler function.
     */
    void operator()(sdbusplus::bus::bus&,
                    sdbusplus::message::message& msg,
                    Zone& zone) const
    {
        std::map<std::string,
                 std::map<std::string,
                          sdbusplus::message::variant<T>>> intfProp;
        sdbusplus::message::object_path op;

        msg.read(op);
        auto objPath = static_cast<const std::string&>(op).c_str();
        if (!objPath || strcmp(objPath, _path))
        {
            // Object path does not match this handler's path
            return;
        }

        msg.read(intfProp);
        auto itIntf = intfProp.find(_iface);
        if (itIntf == intfProp.cend())
        {
            // Interface not found on this handler's path
            return;
        }
        auto itProp = itIntf->second.find(_property);
        if (itProp == itIntf->second.cend())
        {
            // Property not found on this handler's path
            return;
        }

        _handler(zone, std::forward<T>(itProp->second.template get<T>()));
    }

private:
    const char* _path;
    const char* _iface;
    const char* _property;
    U _handler;
};

/**
 * @brief Used to process a Dbus interface added signal event
 *
 * @param[in] path - Object path
 * @param[in] iface - Object interface
 * @param[in] property - Object property
 * @param[in] handler - Handler function to perform
 *
 * @tparam T - The type of the property
 * @tparam U - The type of the handler
 */
template <typename T, typename U>
auto objectSignal(const char* path,
                  const char* iface,
                  const char* property,
                  U&& handler)
{
    return InterfaceAdded<T, U>(path,
                                iface,
                                property,
                                std::forward<U>(handler));
}

} // namespace control
} // namespace fan
} // namespace phosphor
