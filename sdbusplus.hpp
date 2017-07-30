#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/bus/match.hpp>
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace fan
{
namespace util
{
namespace detail
{
namespace errors = sdbusplus::xyz::openbmc_project::Common::Error;
} // namespace detail

/** @brief Alias for PropertiesChanged signal callbacks. */
template <typename ...T>
using Properties = std::map<std::string, sdbusplus::message::variant<T...>>;

/** @class SDBusPlus
 *  @brief DBus access delegate implementation for sdbusplus.
 */
class SDBusPlus
{

    public:
        /** @brief Get the bus connection. */
        static auto& getBus() __attribute__((pure))
        {
            static auto bus = sdbusplus::bus::new_default();
            return bus;
        }

        /** @brief Invoke a method. */
        template <typename ...Args>
        static auto callMethod(
            sdbusplus::bus::bus& bus,
            const std::string& busName,
            const std::string& path,
            const std::string& interface,
            const std::string& method,
            Args&& ... args)
        {
            auto reqMsg = bus.new_method_call(
                    busName.c_str(),
                    path.c_str(),
                    interface.c_str(),
                    method.c_str());
            reqMsg.append(std::forward<Args>(args)...);
            auto respMsg = bus.call(reqMsg);

            if (respMsg.is_method_error())
            {
                phosphor::logging::log<phosphor::logging::level::INFO>(
                        "Failed to invoke DBus method.",
                        phosphor::logging::entry("PATH=%s", path.c_str()),
                        phosphor::logging::entry(
                                "INTERFACE=%s", interface.c_str()),
                        phosphor::logging::entry("METHOD=%s", method.c_str()));
                phosphor::logging::elog<detail::errors::InternalFailure>();
            }

            return respMsg;
        }

        /** @brief Invoke a method. */
        template <typename ...Args>
        static auto callMethod(
            const std::string& busName,
            const std::string& path,
            const std::string& interface,
            const std::string& method,
            Args&& ... args)
        {
            return callMethod(
                    getBus(),
                    busName,
                    path,
                    interface,
                    method,
                    std::forward<Args>(args)...);
        }

        /** @brief Invoke a method and read the response. */
        template <typename Ret, typename ...Args>
        static auto callMethodAndRead(
            sdbusplus::bus::bus& bus,
            const std::string& busName,
            const std::string& path,
            const std::string& interface,
            const std::string& method,
            Args&& ... args)
        {
            sdbusplus::message::message respMsg =
                    callMethod<Args...>(
                            bus,
                            busName,
                            path,
                            interface,
                            method,
                            std::forward<Args>(args)...);
            Ret resp;
            respMsg.read(resp);
            return resp;
        }

        /** @brief Invoke a method and read the response. */
        template <typename Ret, typename ...Args>
        static auto callMethodAndRead(
            const std::string& busName,
            const std::string& path,
            const std::string& interface,
            const std::string& method,
            Args&& ... args)
        {
            return callMethodAndRead<Ret>(
                    getBus(),
                    busName,
                    path,
                    interface,
                    method,
                    std::forward<Args>(args)...);
        }


        /** @brief Get service from the mapper. */
        static auto getService(
            sdbusplus::bus::bus& bus,
            const std::string& path,
            const std::string& interface)
        {
            using namespace std::literals::string_literals;
            using GetObject = std::map<std::string, std::vector<std::string>>;

            auto mapperResp = callMethodAndRead<GetObject>(
                    bus,
                    "xyz.openbmc_project.ObjectMapper"s,
                    "/xyz/openbmc_project/object_mapper"s,
                    "xyz.openbmc_project.ObjectMapper"s,
                    "GetObject"s,
                    path,
                    GetObject::mapped_type{interface});

            if (mapperResp.empty())
            {
                phosphor::logging::log<phosphor::logging::level::INFO>(
                        "Object not found.",
                        phosphor::logging::entry("PATH=%s", path.c_str()),
                        phosphor::logging::entry(
                                "INTERFACE=%s", interface.c_str()));
                phosphor::logging::elog<detail::errors::InternalFailure>();
            }
            return mapperResp.begin()->first;
        }

        /** @brief Get service from the mapper. */
        static auto getService(
            const std::string& path,
            const std::string& interface)
        {
            return getService(
                    getBus(),
                    path,
                    interface);
        }

        /** @brief Get a property with mapper lookup. */
        template <typename Property>
        static auto getProperty(
            sdbusplus::bus::bus& bus,
            const std::string& path,
            const std::string& interface,
            const std::string& property)
        {
            using namespace std::literals::string_literals;

            auto msg = callMethod(
                    bus,
                    getService(bus, path, interface),
                    path,
                    "org.freedesktop.DBus.Properties"s,
                    "Get"s,
                    interface,
                    property);
            sdbusplus::message::variant<Property> value;
            msg.read(value);
            return value.template get<Property>();
        }

        /** @brief Get a property with mapper lookup. */
        template <typename Property>
        static auto getProperty(
            const std::string& path,
            const std::string& interface,
            const std::string& property)
        {
            return getProperty<Property>(
                    getBus(),
                    path,
                    interface,
                    property);
        }

        /** @brief Set a property with mapper lookup. */
        template <typename Property>
        static void setProperty(
            sdbusplus::bus::bus& bus,
            const std::string& path,
            const std::string& interface,
            const std::string& property,
            Property&& value)
        {
            using namespace std::literals::string_literals;

            sdbusplus::message::variant<Property> varValue(
                    std::forward<Property>(value));

            callMethod(
                    bus,
                    getService(bus, path, interface),
                    path,
                    "org.freedesktop.DBus.Properties"s,
                    "Set"s,
                    interface,
                    property,
                    varValue);
        }

        /** @brief Set a property with mapper lookup. */
        template <typename Property>
        static void setProperty(
            const std::string& path,
            const std::string& interface,
            const std::string& property,
            Property&& value)
        {
            return setProperty(
                    getBus(),
                    path,
                    interface,
                    property,
                    std::forward<Property>(value));
        }

        /** @brief Invoke method with mapper lookup. */
        template <typename ...Args>
        static auto lookupAndCallMethod(
            sdbusplus::bus::bus& bus,
            const std::string& path,
            const std::string& interface,
            const std::string& method,
            Args&& ... args)
        {
            return callMethod(
                    bus,
                    getService(bus, path, interface),
                    path,
                    interface,
                    method,
                    std::forward<Args>(args)...);
        }

        /** @brief Invoke method with mapper lookup. */
        template <typename ...Args>
        static auto lookupAndCallMethod(
            const std::string& path,
            const std::string& interface,
            const std::string& method,
            Args&& ... args)
        {
            return lookupAndCallMethod(
                    getBus(),
                    path,
                    interface,
                    method,
                    std::forward<Args>(args)...);
        }

        /** @brief Invoke method and read with mapper lookup. */
        template <typename Ret, typename ...Args>
        static auto lookupCallMethodAndRead(
            sdbusplus::bus::bus& bus,
            const std::string& path,
            const std::string& interface,
            const std::string& method,
            Args&& ... args)
        {
            return callMethodAndRead(
                    bus,
                    getService(bus, path, interface),
                    path,
                    interface,
                    method,
                    std::forward<Args>(args)...);
        }

        /** @brief Invoke method and read with mapper lookup. */
        template <typename Ret, typename ...Args>
        static auto lookupCallMethodAndRead(
            const std::string& path,
            const std::string& interface,
            const std::string& method,
            Args&& ... args)
        {
            return lookupCallMethodAndRead<Ret>(
                    getBus(),
                    path,
                    interface,
                    method,
                    std::forward<Args>(args)...);
        }
};

} // namespace util
} // namespace fan
} // namespace phosphor
