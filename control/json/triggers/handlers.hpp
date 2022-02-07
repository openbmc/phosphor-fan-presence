#pragma once

#include "../manager.hpp"

#include <sdbusplus/message.hpp>

#include <algorithm>
#include <map>
#include <tuple>
#include <vector>

namespace phosphor::fan::control::json::trigger::signal
{

using namespace sdbusplus::message;

struct Handlers
{

  public:
    /**
     * @brief Processes a properties changed signal and updates the property's
     * value in the manager's object cache
     *
     * @param[in] msg - The sdbusplus signal message
     * @param[in] obj - Object data associated with the signal
     * @param[in] mgr - Manager that stores the object cache
     */
    static bool propertiesChanged(message& msg, const SignalObject& obj,
                                  Manager& mgr)
    {
        std::string intf;
        msg.read(intf);
        if (intf != std::get<Intf>(obj))
        {
            // Interface name does not match object's interface
            return false;
        }

        std::map<std::string, PropertyVariantType> props;
        msg.read(props);
        auto itProp = props.find(std::get<Prop>(obj));
        if (itProp == props.cend())
        {
            // Object's property not in dictionary of properties changed
            return false;
        }

        mgr.setProperty(std::get<Path>(obj), std::get<Intf>(obj),
                        std::get<Prop>(obj), itProp->second);
        return true;
    }

    /**
     * @brief Processes an interfaces added signal and adds the interface
     * (including property & property value) to the manager's object cache
     *
     * @param[in] msg - The sdbusplus signal message
     * @param[in] obj - Object data associated with the signal
     * @param[in] mgr - Manager that stores the object cache
     */
    static bool interfacesAdded(message& msg, const SignalObject& obj,
                                Manager& mgr)
    {
        sdbusplus::message::object_path op;
        msg.read(op);
        if (static_cast<const std::string&>(op) != std::get<Path>(obj))
        {
            // Path name does not match object's path
            return false;
        }

        std::map<std::string, std::map<std::string, PropertyVariantType>>
            intfProps;
        msg.read(intfProps);
        auto itIntf = intfProps.find(std::get<Intf>(obj));
        if (itIntf == intfProps.cend())
        {
            // Object's interface not in dictionary of interfaces added
            return false;
        }

        auto itProp = itIntf->second.find(std::get<Prop>(obj));
        if (itProp == itIntf->second.cend())
        {
            // Object's property not in dictionary of properties of interface
            return false;
        }

        mgr.setProperty(std::get<Path>(obj), std::get<Intf>(obj),
                        std::get<Prop>(obj), itProp->second);
        return true;
    }

    /**
     * @brief Processes an interfaces removed signal and removes the interface
     * (including its properties) from the object cache on the manager
     *
     * @param[in] msg - The sdbusplus signal message
     * @param[in] obj - Object data associated with the signal
     * @param[in] mgr - Manager that stores the object cache
     */
    static bool interfacesRemoved(message& msg, const SignalObject& obj,
                                  Manager& mgr)
    {
        sdbusplus::message::object_path op;
        msg.read(op);
        if (static_cast<const std::string&>(op) != std::get<Path>(obj))
        {
            // Path name does not match object's path
            return false;
        }

        std::vector<std::string> intfs;
        msg.read(intfs);
        auto itIntf =
            std::find(intfs.begin(), intfs.end(), std::get<Intf>(obj));
        if (itIntf == intfs.cend())
        {
            // Object's interface not in list of interfaces removed
            return false;
        }

        mgr.removeInterface(std::get<Path>(obj), std::get<Intf>(obj));
        return true;
    }

    /**
     * @brief Processes a name owner changed signal and updates the service's
     * owner state for all objects/interfaces associated in the cache
     *
     * @param[in] msg - The sdbusplus signal message
     * @param[in] mgr - Manager that stores the service's owner state
     */
    static bool nameOwnerChanged(message& msg, const SignalObject&,
                                 Manager& mgr)
    {
        bool hasOwner = false;

        std::string serv;
        msg.read(serv);

        std::string oldOwner;
        msg.read(oldOwner);

        std::string newOwner;
        msg.read(newOwner);
        if (!newOwner.empty())
        {
            hasOwner = true;
        }

        mgr.setOwner(serv, hasOwner);
        return true;
    }

    /**
     * @brief Processes a dbus member signal, there is nothing associated or
     * any cache to update when this signal is received
     */
    static bool member(message&, const SignalObject&, Manager&)
    {
        return true;
    }
};

} // namespace phosphor::fan::control::json::trigger::signal
