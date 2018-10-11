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
 * @brief A match filter functor for Dbus property values
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
    Properties(Group&& group, U&& handler) :
        _group(group),
        _handler(std::forward<U>(handler)) { }

    /**
     * @brief Run signal handler function
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
            // PropertiesChanged messages are against single membered groups
            auto member = _group.front();

            std::string intf;
            std::map<std::string, sdbusplus::message::variant<T>> props;

            msg.read(intf);
            if (intf != std::get<intfPos>(member))
            {
                // Interface name does not match on object
                return;
            }

            msg.read(props);
            auto it = props.find(std::get<propPos>(member));
            if (it == props.cend())
            {
                // Property not included in dictionary of properties changed
                return;
            }

            _handler(zone,
                     std::get<pathPos>(member),
                     std::get<intfPos>(member),
                     std::get<propPos>(member),
                     std::forward<T>(it->second.template get<T>()));
        }
    }

    /**
     * @brief Run init handler function
     *
     * Get the property from each member object of the group
     * and run the handler function.
     */
    void operator()(Zone& zone, const Group& group) const
    {
        auto grp = group;
        if (!_group.empty())
        {
            // Use group subset when provided
            grp = _group;
        }
        std::for_each(
            grp.begin(),
            grp.end(),
            [&zone, handler = std::move(_handler)](auto const& member)
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
                    handler(zone, path, intf, prop, std::forward<T>(val));
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
    const Group _group;
    U _handler;
};

/**
 * @brief Used to process a Dbus properties changed signal event
 *
 * @param[in] path - Object path
 * @param[in] intf - Object interface
 * @param[in] prop - Object property
 * @param[in] handler - Handler function to perform
 *
 * @tparam T - The type of the property
 * @tparam U - The type of the handler
 */
template <typename T, typename U>
auto propertiesChanged(const char* path,
                       const char* intf,
                       const char* prop,
                       U&& handler)
{
    Group group = {{path, intf, prop}};
    return Properties<T, U>(std::move(group), std::forward<U>(handler));
}

/**
 * @brief Used to get the properties of a group
 *
 * @param[in] handler - Handler function to perform
 * @param[in] group - Group subset of objects to get properties from (optional)
 *
 * @tparam T - The type of all the properties
 * @tparam U - The type of the handler
 */
template <typename T, typename U>
auto getProperties(Group&& group, U&& handler)
{
    return Properties<T, U>(std::move(group), std::forward<U>(handler));
}

/**
 * @struct Interfaces Added
 * @brief A match filter functor for Dbus interfaces added signals
 *
 * @tparam T - The type of the property value
 * @tparam U - The type of the handler
 */
template <typename T, typename U>
struct InterfaceAdded
{
    InterfacesAdded() = delete;
    ~InterfacesAdded() = default;
    InterfacesAdded(const InterfacesAdded&) = default;
    InterfacesAdded& operator=(const InterfacesAdded&) = default;
    InterfacesAdded(InterfacesAdded&&) = default;
    InterfacesAdded& operator=(InterfacesAdded&&) = default;
    InterfacesAdded(const char* path,
                    const char* intf,
                    const char* prop,
                    U&& handler) :
        _path(path),
        _intf(intf),
        _prop(prop),
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
            auto itIntf = intfProp.find(_intf);
            if (itIntf == intfProp.cend())
            {
                // Interface not found on this handler's path
                return;
            }
            auto itProp = itIntf->second.find(_prop);
            if (itProp == itIntf->second.cend())
            {
                // Property not found on this handler's path
                return;
            }

            _handler(zone, _path, _intf, _prop,
                     std::forward<T>(itProp->second.template get<T>()));
        }
    }

private:
    const char* _path;
    const char* _intf;
    const char* _prop;
    U _handler;
};

/**
 * @brief Used to process a Dbus interfaces added signal event
 *
 * @param[in] path - Object path
 * @param[in] intf - Object interface
 * @param[in] prop - Object property
 * @param[in] handler - Handler function to perform
 *
 * @tparam T - The type of the property
 * @tparam U - The type of the handler
 */
template <typename T, typename U>
auto interfacesAdded(const char* path,
                     const char* intf,
                     const char* prop,
                     U&& handler)
{
    return InterfacesAdded<T, U>(path,
                                 intf,
                                 prop,
                                 std::forward<U>(handler));
}

/**
 * @struct Interfaces Removed
 * @brief A match filter functor for Dbus interfaces removed signals
 *
 * @tparam U - The type of the handler
 */
template <typename U>
struct InterfaceRemoved
{
    InterfacesRemoved() = delete;
    ~InterfacesRemoved() = default;
    InterfacesRemoved(const InterfacesRemoved&) = default;
    InterfacesRemoved& operator=(const InterfacesRemoved&) = default;
    InterfacesRemoved(InterfacesRemoved&&) = default;
    InterfacesRemoved& operator=(InterfacesRemoved&&) = default;
    InterfacesRemoved(const char* path,
                      const char* intf,
                      U&& handler) :
        _path(path),
        _intf(intf),
        _handler(std::forward<U>(handler)) { }

    /** @brief Run signal handler function
     *
     * Extract the interfaces from the InterfacesRemoved
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
            auto itIntf = std::find(intfs.begin(), intfs.end(), _intf);
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
    const char* _intf;
    U _handler;
};

/**
 * @brief Used to process a Dbus interfaces removed signal event
 *
 * @param[in] path - Object path
 * @param[in] intf - Object interface
 * @param[in] handler - Handler function to perform
 *
 * @tparam U - The type of the handler
 */
template <typename U>
auto interfacesRemoved(const char* path,
                       const char* intf,
                       U&& handler)
{
    return InterfacesRemoved<U>(path,
                                intf,
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
    NameOwner(Group&& group, U&& handler) :
        _group(group),
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
            _handler(zone, _group, name, hasOwner);
        }
    }

    void operator()(Zone& zone, const Group& group) const
    {
        std::string name = "";
        bool hasOwner = false;

        auto grp = group;
        if (!_group.empty())
        {
            // Use group subset when provided
            grp = _group;
        }
        std::for_each(
            grp.begin(),
            grp.end(),
            [&zone, &grp, &name, &hasOwner, handler = std::move(_handler)](
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
                        handler(zone, grp, name, hasOwner);
                    }
                }
                catch (const util::DBusMethodError& e)
                {
                    // Failed to get service name owner state
                    name = "";
                    hasOwner = false;
                    handler(zone, grp, name, hasOwner);
                }
            }
        );
    }

private:
    const Group _group;
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
auto nameOwnerChanged(Group&& group, U&& handler)
{
    return NameOwner<U>(std::move(group), std::forward<U>(handler));
}

/**
 * @brief Used to process the init of a name owner event
 *
 * @param[in] handler - Handler function to perform
 * @param[in] group -
 *
 * @tparam U - The type of the handler
 *
 * @return - The NameOwnerChanged signal struct
 */
template <typename U>
auto nameHasOwner(Group&& group, U&& handler)
{
    return NameOwner<U>(std::move(group), std::forward<U>(handler));
}

} // namespace control
} // namespace fan
} // namespace phosphor
