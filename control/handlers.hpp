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
 * @brief A handler function to set/update service name owners
 * @details Sets or updates service name owners used by a group with a blank
 * service owner representing a service no longer exists
 *
 * @param[in] group - Group associated with a service
 *
 * @return Lambda function
 *     A lambda function to set/update the service name owners
 */
auto setService(const auto&& group)
{
    return [group = std::move(group)](auto& zone, auto& name, auto& owner)
    {
        // Update service name owner list of a group
        zone.setServiceOwner(&group, name, owner);
    };
}

} // namespace handler
} // namespace control
} // namespace fan
} // namespace phosphor
