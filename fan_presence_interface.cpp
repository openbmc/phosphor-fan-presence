#include <sdbusplus/bus.hpp>
#include "fan_presence_interface.hpp"

namespace phosphor
{
namespace fan
{
namespace presence
{

    int getTach(const std::string& objpath, const std::string& iface)
    {
        int64_t tach;

        auto bus = sdbusplus::bus::new_default();
        auto method = bus.new_method_call(
                        "xyz.openbmc_project.ObjectMapper",
                        "/xyz/openbmc_project/ObjectMapper",
                        "xyz.openbmc_project.ObjectMapper",
                        "GetObject");
        method.append(objpath);
        method.append(std::vector<std::string>({iface}));

        auto methodResponseMsg = bus.call(method);
        std::map<std::string, std::vector<std::string>> methodResponse;
        methodResponseMsg.read(methodResponse);
        if (methodResponseMsg.is_method_error())
        {
            //TODO Retry or throw exception/log error?
        }

        auto host = methodResponse.begin()->first;
        method = bus.new_method_call(
                    host.c_str(),
                    objpath.c_str(),
                    "org.freedesktop.DBus.Properties",
                    "Get");
        method.append(iface);
        method.append("value");

        methodResponseMsg = bus.call(method);
        methodResponseMsg.read(tach);

        return tach;
    }

} // namespace presence
} // namespace fan
} // namespace phosphor
