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

} // namespace handler
} // namespace control
} // namespace fan
} // namespace phosphor
