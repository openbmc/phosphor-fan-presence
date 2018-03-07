#pragma once

namespace phosphor
{
namespace fan
{
namespace control
{
namespace handler
{

/**
 * @brief A handler function to set/update a property
 * @details Sets or updates a property's value determined by a combination of
 * an object's path and property names
 *
 * @param[in] path - Object's path name
 * @param[in] interface - Object's interface name
 * @param[in] property - Object's property name
 *
 * @return Lambda function
 *     A lambda function to set/update the property value
 */
template <typename T>
auto setProperty(const char* path, const char* interface, const char* property)
{
    return [=](auto& zone, T&& arg)
    {
        zone.setPropertyValue(path, interface, property, std::forward<T>(arg));
    };
}

/**
 * @brief A handler function to set/update service name owner state
 * @details Sets or updates service name owner state used by a group where
 * a service name without an owner represents the service no longer exists
 *
 * @param[in] group - Group associated with a service
 *
 * @return Lambda function
 *     A lambda function to set/update the service name owner state
 */
auto setService(Group&& group)
{
    return [group = std::move(group)](auto& zone, auto& name, bool hasOwner)
    {
        // Update service name owner state list of a group
        zone.setServiceOwner(&group, name, hasOwner);
    };
}

/**
 * @brief A handler function to remove an interface from an object path
 * @details Removes an interface from an object's path which includes removing
 * all properties that would be under that interface
 *
 * @param[in] path - Object's path name
 * @param[in] interface - Object's interface name
 *
 * @return Lambda function
 *     A lambda function to remove the interface
 */
auto removeInterface(const char* path, const char* interface)
{
    return[=](auto& zone)
    {
        zone.removeObjectInterface(path, interface);
    };
}

/**
 * @brief A handler function to read and update property values
 * @details Reads and updates each group member's property value
 * from the given group
 *
 * @param[in] group - Group of objects to read/update property values for
 *
 * @return Lambda function
 *      A lambda function to read and update each group member's property value
 */
template <typename T>
auto updateProperty(Group&& group)
{
    return [group = std::move(group)](auto& bus,
                                      sdbusplus::message::message& msg,
                                      auto& zone)
    {
        try
        {
            std::for_each(
                group.begin(),
                group.end(),
                [&bus, &zone](auto const& member)
                {
                    auto path = std::get<pathPos>(member);
                    auto intf = std::get<intfPos>(member);
                    auto prop = std::get<propPos>(member);
                    auto serv = zone.getService(path, intf);
                    auto val = util::SDBusPlus::getProperty<T>(bus,
                                                               serv,
                                                               path,
                                                               intf,
                                                               prop);
                    zone.setPropertyValue(path.c_str(),
                                          intf.c_str(),
                                          prop.c_str(),
                                          val);
                }
            );
        }
        catch (const sdbusplus::exception::SdBusError&)
        {
            // Property will not be updated or available
        }
        catch (const util::DBusError&)
        {
            // Property will not be updated or available
        }
    };
}

} // namespace handler
} // namespace control
} // namespace fan
} // namespace phosphor
