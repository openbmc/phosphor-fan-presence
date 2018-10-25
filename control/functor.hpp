#pragma once

#include "sdbusplus.hpp"
#include "types.hpp"

#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace fan
{
namespace control
{
class Zone;

using namespace phosphor::fan;
using namespace sdbusplus::bus::match;
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
    PropertyChanged(const char* path, const char* iface, const char* property,
                    U&& handler) :
        _path(path),
        _iface(iface), _property(property), _handler(std::forward<U>(handler))
    {
    }

    /** @brief Run signal handler function
     *
     * Extract the property from the PropertiesChanged
     * message (or read the property when the message is null)
     * and run the handler function.
     */
    void operator()(sdbusplus::bus::bus& bus, sdbusplus::message::message& msg,
                    Zone& zone) const
    {
        if (msg)
        {
            std::map<std::string, sdbusplus::message::variant<T>> properties;
            std::string iface;

            msg.read(iface);
            if (iface != _iface)
            {
                return;
            }

            msg.read(properties);
            auto it = properties.find(_property);
            if (it == properties.cend())
            {
                log<level::ERR>("Unable to find property on interface",
                                entry("PROPERTY=%s", _property),
                                entry("INTERFACE=%s", _iface),
                                entry("PATH=%s", _path));
                return;
            }

            _handler(zone, std::forward<T>(it->second.template get<T>()));
        }
        else
        {
            try
            {
                auto service = zone.getService(_path, _iface);
                auto val = util::SDBusPlus::getProperty<T>(bus, service, _path,
                                                           _iface, _property);
                _handler(zone, std::forward<T>(val));
            }
            catch (const sdbusplus::exception::SdBusError&)
            {
                // Property will not be used unless a property changed
                // signal message is received for this property.
            }
            catch (const util::DBusError&)
            {
                // Property will not be used unless a property changed
                // signal message is received for this property.
            }
        }
    }

  private:
    const char* _path;
    const char* _iface;
    const char* _property;
    U _handler;
};

/**
 * @brief Used to process a Dbus property changed signal event
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
auto propertySignal(const char* path, const char* iface, const char* property,
                    U&& handler)
{
    return PropertyChanged<T, U>(path, iface, property,
                                 std::forward<U>(handler));
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
    InterfaceAdded(const char* path, const char* iface, const char* property,
                   U&& handler) :
        _path(path),
        _iface(iface), _property(property), _handler(std::forward<U>(handler))
    {
    }

    /** @brief Run signal handler function
     *
     * Extract the property from the InterfacesAdded
     * message and run the handler function.
     */
    void operator()(sdbusplus::bus::bus&, sdbusplus::message::message& msg,
                    Zone& zone) const
    {
        if (msg)
        {
            std::map<std::string,
                     std::map<std::string, sdbusplus::message::variant<T>>>
                intfProp;
            sdbusplus::message::object_path op;

            msg.read(op);
            if (static_cast<const std::string&>(op) != _path)
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
auto objectSignal(const char* path, const char* iface, const char* property,
                  U&& handler)
{
    return InterfaceAdded<T, U>(path, iface, property,
                                std::forward<U>(handler));
}

/**
 * @struct Interface Removed
 * @brief A match filter functor for Dbus interface removed signals
 *
 * @tparam U - The type of the handler
 */
template <typename U>
struct InterfaceRemoved
{
    InterfaceRemoved() = delete;
    ~InterfaceRemoved() = default;
    InterfaceRemoved(const InterfaceRemoved&) = default;
    InterfaceRemoved& operator=(const InterfaceRemoved&) = default;
    InterfaceRemoved(InterfaceRemoved&&) = default;
    InterfaceRemoved& operator=(InterfaceRemoved&&) = default;
    InterfaceRemoved(const char* path, const char* iface, U&& handler) :
        _path(path), _iface(iface), _handler(std::forward<U>(handler))
    {
    }

    /** @brief Run signal handler function
     *
     * Extract the property from the InterfacesRemoved
     * message and run the handler function.
     */
    void operator()(sdbusplus::bus::bus&, sdbusplus::message::message& msg,
                    Zone& zone) const
    {
        if (msg)
        {
            std::vector<std::string> intfs;
            sdbusplus::message::object_path op;

            msg.read(op);
            if (static_cast<const std::string&>(op) != _path)
            {
                // Object path does not match this handler's path
                return;
            }

            msg.read(intfs);
            auto itIntf = std::find(intfs.begin(), intfs.end(), _iface);
            if (itIntf == intfs.cend())
            {
                // Interface not found on this handler's path
                return;
            }

            _handler(zone);
        }
    }

  private:
    const char* _path;
    const char* _iface;
    U _handler;
};

/**
 * @brief Used to process a Dbus interface removed signal event
 *
 * @param[in] path - Object path
 * @param[in] iface - Object interface
 * @param[in] handler - Handler function to perform
 *
 * @tparam U - The type of the handler
 */
template <typename U>
auto objectSignal(const char* path, const char* iface, U&& handler)
{
    return InterfaceRemoved<U>(path, iface, std::forward<U>(handler));
}

/**
 * @struct Name Owner Changed
 * @brief A match filter functor for Dbus name owner changed signals
 *
 * @tparam U - The type of the handler
 */
template <typename U>
struct NameOwnerChanged
{
    NameOwnerChanged() = delete;
    ~NameOwnerChanged() = default;
    NameOwnerChanged(const NameOwnerChanged&) = default;
    NameOwnerChanged& operator=(const NameOwnerChanged&) = default;
    NameOwnerChanged(NameOwnerChanged&&) = default;
    NameOwnerChanged& operator=(NameOwnerChanged&&) = default;
    NameOwnerChanged(const char* path, const char* iface, U&& handler) :
        _path(path), _iface(iface), _handler(std::forward<U>(handler))
    {
    }

    /** @brief Run signal handler function
     *
     * Extract the name owner from the NameOwnerChanged
     * message (or read the name owner when the message is null)
     * and run the handler function.
     */
    void operator()(sdbusplus::bus::bus& bus, sdbusplus::message::message& msg,
                    Zone& zone) const
    {
        std::string name;
        bool hasOwner = false;
        if (msg)
        {
            // Handle NameOwnerChanged signals
            msg.read(name);

            std::string oldOwn;
            msg.read(oldOwn);

            std::string newOwn;
            msg.read(newOwn);
            if (!newOwn.empty())
            {
                hasOwner = true;
            }
        }
        else
        {
            try
            {
                // Initialize NameOwnerChanged data store with service name
                name = zone.getService(_path, _iface);
                hasOwner = util::SDBusPlus::callMethodAndRead<bool>(
                    bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
                    "org.freedesktop.DBus", "NameHasOwner", name);
            }
            catch (const util::DBusMethodError& e)
            {
                // Failed to get service name owner state
                hasOwner = false;
            }
        }

        _handler(zone, name, hasOwner);
    }

  private:
    const char* _path;
    const char* _iface;
    U _handler;
};

/**
 * @brief Used to process a Dbus name owner changed signal event
 *
 * @param[in] path - Object path
 * @param[in] iface - Object interface
 * @param[in] handler - Handler function to perform
 *
 * @tparam U - The type of the handler
 *
 * @return - The NameOwnerChanged signal struct
 */
template <typename U>
auto ownerSignal(const char* path, const char* iface, U&& handler)
{
    return NameOwnerChanged<U>(path, iface, std::forward<U>(handler));
}

} // namespace control
} // namespace fan
} // namespace phosphor
