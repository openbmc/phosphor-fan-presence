#pragma once

#include "types.hpp"
#include "sdbusplus.hpp"
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
 * @brief Create a trigger function object
 *
 * @param[in] trigger - The trigger being created
 *
 * @return - The created trigger function object
 */
template <typename T>
auto make_trigger(T&& trigger)
{
    return Trigger(std::forward<T>(trigger));
}

/**
 * @brief Create a handler function object
 *
 * @param[in] handler - The handler being created
 *
 * @return - The created handler function object
 */
template <typename T, typename U>
auto make_handler(U&& handler)
{
    return T(std::forward<U>(handler));
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
struct Properties
{
    Properties() = delete;
    ~Properties() = default;
    Properties(const Properties&) = default;
    Properties& operator=(const Properties&) = default;
    Properties(Properties&&) = default;
    Properties& operator=(Properties&&) = default;
    explicit Properties(U&& handler) :
        _path(""),
        _iface(""),
        _property(""),
        _handler(std::forward<U>(handler)) { }
    Properties(const char* path,
               const char* iface,
               const char* property,
               U&& handler) :
        _path(path),
        _iface(iface),
        _property(property),
        _handler(std::forward<U>(handler)) { }

    /** @brief Run signal handler function
     *
     * Extract the property from the PropertiesChanged
     * message and run the handler function.
     */
    void operator()(sdbusplus::bus::bus& bus,
                    sdbusplus::message::message& msg,
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
    }

    /** @brief Run init handler function
     *
     * Get the property from each member object of the group
     * and run the handler function.
     */
    void operator()(Zone& zone, const Group& group) const
    {
        std::for_each(
            group.begin(),
            group.end(),
            [&zone, &group, handler = std::move(_handler)](auto const& member)
            {
                auto path = std::get<pathPos>(member);
                auto intf = std::get<intfPos>(member);
                auto prop = std::get<propPos>(member);
                try
                {
                    auto service = zone.getService(path, intf);
                    auto val = util::SDBusPlus::getProperty<T>(zone.getBus(),
                                                               service,
                                                               path,
                                                               intf,
                                                               prop);
                    handler(zone, std::forward<T>(val));
                }
                catch (const sdbusplus::exception::SdBusError&)
                {
                    // Property value not sent to handler
                }
                catch (const util::DBusError&)
                {
                    // Property value not sent to handler
                }
            }
        );
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
auto propertiesChanged(const char* path,
                       const char* iface,
                       const char* property,
                       U&& handler)
{
    return Properties<T, U>(path,
                            iface,
                            property,
                            std::forward<U>(handler));
}

/**
 * @brief Used to get the property value of an object
 *
 * @param[in] handler - Handler function to perform
 *
 * @tparam T - The type of the property
 * @tparam U - The type of the handler
 */
template <typename T, typename U>
auto getProperty(U&& handler)
{
    return Properties<T, U>(std::forward<U>(handler));
}

/**
 * @struct Interface Added
 * @brief A match filter functor for Dbus interfaces added signals
 *
 * @tparam T - The type of the property value
 * @tparam U - The type of the handler
 */
template <typename T, typename U>
struct InterfacesAdded
{
    InterfacesAdded() = delete;
    ~InterfacesAdded() = default;
    InterfacesAdded(const InterfacesAdded&) = default;
    InterfacesAdded& operator=(const InterfacesAdded&) = default;
    InterfacesAdded(InterfacesAdded&&) = default;
    InterfacesAdded& operator=(InterfacesAdded&&) = default;
    InterfacesAdded(const char* path,
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
        if (msg)
        {
            std::map<std::string,
                     std::map<std::string,
                              sdbusplus::message::variant<T>>> intfProp;
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
 * @brief Used to process a Dbus interfaces added signal event
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
auto interfacesAdded(const char* path,
                     const char* iface,
                     const char* property,
                     U&& handler)
{
    return InterfacesAdded<T, U>(path,
                                 iface,
                                 property,
                                 std::forward<U>(handler));
}

/**
 * @struct Interface Removed
 * @brief A match filter functor for Dbus interfaces removed signals
 *
 * @tparam U - The type of the handler
 */
template <typename U>
struct InterfacesRemoved
{
    InterfacesRemoved() = delete;
    ~InterfacesRemoved() = default;
    InterfacesRemoved(const InterfacesRemoved&) = default;
    InterfacesRemoved& operator=(const InterfacesRemoved&) = default;
    InterfacesRemoved(InterfacesRemoved&&) = default;
    InterfacesRemoved& operator=(InterfacesRemoved&&) = default;
    InterfacesRemoved(const char* path,
                      const char* iface,
                      U&& handler) :
        _path(path),
        _iface(iface),
        _handler(std::forward<U>(handler)) { }

    /** @brief Run signal handler function
     *
     * Extract the property from the InterfacesRemoved
     * message and run the handler function.
     */
    void operator()(sdbusplus::bus::bus&,
                    sdbusplus::message::message& msg,
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
 * @brief Used to process a Dbus interfaces removed signal event
 *
 * @param[in] path - Object path
 * @param[in] iface - Object interface
 * @param[in] handler - Handler function to perform
 *
 * @tparam U - The type of the handler
 */
template <typename U>
auto interfacesRemoved(const char* path,
                       const char* iface,
                       U&& handler)
{
    return InterfacesRemoved<U>(path,
                                iface,
                                std::forward<U>(handler));
}

/**
 * @struct Name Owner
 * @brief A functor for Dbus name owner signals and methods
 *
 * @tparam U - The type of the handler
 */
template <typename U>
struct NameOwner
{
    NameOwner() = delete;
    ~NameOwner() = default;
    NameOwner(const NameOwner&) = default;
    NameOwner& operator=(const NameOwner&) = default;
    NameOwner(NameOwner&&) = default;
    NameOwner& operator=(NameOwner&&) = default;
    explicit NameOwner(U&& handler) :
        _handler(std::forward<U>(handler)) { }

    /** @brief Run signal handler function
     *
     * Extract the name owner from the NameOwnerChanged
     * message and run the handler function.
     */
    void operator()(sdbusplus::bus::bus& bus,
                    sdbusplus::message::message& msg,
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
            _handler(zone, name, hasOwner);
        }
    }

    void operator()(Zone& zone,
                    const Group& group) const
    {
        std::string name = "";
        bool hasOwner = false;
        std::for_each(
            group.begin(),
            group.end(),
            [&zone, &group, &name, &hasOwner, handler = std::move(_handler)](
                auto const& member)
            {
                auto path = std::get<pathPos>(member);
                auto intf = std::get<intfPos>(member);
                try
                {
                    auto servName = zone.getService(path, intf);
                    if (name != servName)
                    {
                        name = servName;
                        hasOwner = util::SDBusPlus::callMethodAndRead<bool>(
                                zone.getBus(),
                                "org.freedesktop.DBus",
                                "/org/freedesktop/DBus",
                                "org.freedesktop.DBus",
                                "NameHasOwner",
                                name);
                        // Update service name owner state list of a group
                        handler(zone, name, hasOwner);
                    }
                }
                catch (const util::DBusMethodError& e)
                {
                    // Failed to get service name owner state
                    name = "";
                    hasOwner = false;
                }

            }
        );
    }

private:
    U _handler;
};

/**
 * @brief Used to process a Dbus name owner changed signal event
 *
 * @param[in] handler - Handler function to perform
 *
 * @tparam U - The type of the handler
 *
 * @return - The NameOwnerChanged signal struct
 */
template <typename U>
auto nameOwnerChanged(U&& handler)
{
    return NameOwner<U>(std::forward<U>(handler));
}

/**
 * @brief Used to process the init of a name owner event
 *
 * @param[in] handler - Handler function to perform
 *
 * @tparam U - The type of the handler
 *
 * @return - The NameOwnerChanged signal struct
 */
template <typename U>
auto nameHasOwner(U&& handler)
{
    return NameOwner<U>(std::forward<U>(handler));
}

} // namespace control
} // namespace fan
} // namespace phosphor
