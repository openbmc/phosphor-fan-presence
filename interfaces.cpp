#include <sdbusplus/bus.hpp>
#include "interfaces.hpp"

namespace phosphor
{
namespace fan
{
namespace presence
{

    //TODO Should get these from phosphor-objmgr config.h
    constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
    constexpr auto MAPPER_PATH = "/xyz/openbmc_project/ObjectMapper";
    constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";

    //TODO Able to return `auto` here?
    int64_t getTach(const std::string& objpath, const std::string& iface)
    {
        int64_t tach = 0;
        std::map<std::string, std::vector<std::string>> methodResponse;

        auto bus = sdbusplus::bus::new_default();
        auto method = bus.new_method_call(
                        MAPPER_BUSNAME,
                        MAPPER_PATH,
                        MAPPER_INTERFACE,
                        "GetObject");
        method.append(objpath);
        method.append(std::vector<std::string>({iface}));

        auto methodResponseMsg = bus.call(method);
        if (methodResponseMsg.is_method_error())
        {
            //TODO Retry or throw exception/log error?
        }
        methodResponseMsg.read(methodResponse);

        auto host = methodResponse.begin()->first;
        method = bus.new_method_call(
                    host.c_str(),
                    objpath.c_str(),
                    "org.freedesktop.DBus.Properties",
                    "Get");
        method.append(iface);
        method.append("Value");

        methodResponseMsg = bus.call(method);
        if (methodResponseMsg.is_method_error())
        {
            //TODO Retry or throw exception/log error?
        }
        methodResponseMsg.read(tach);

        return tach;
    }

} // namespace presence
} // namespace fan
} // namespace phosphor
