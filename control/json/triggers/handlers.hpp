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
};

} // namespace phosphor::fan::control::json::trigger::signal
