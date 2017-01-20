#include <sdbusplus/bus.hpp>
#include "fan_presence_interface.hpp"

namespace phosphor
{
namespace fan
{
namespace presence
{

    auto getTach(std::string objpath, std::string interf)
    {
        int64_t tach;

        auto bus = sdbusplus::bus::new_default();
        auto method = bus.new_method_call(
                        "xyz.openbmc_project.ObjectMapper",
                        "/xyz/openbmc_project/ObjectMapper",
                        "xyz.openbmc_project.ObjectMapper",
                        "GetObject");
        method.append(objpath);
        method.append(std::vector<std::string>({interf}));

        auto methodResponseMsg = bus.call(method);
        std::map<std::string, std::vector<std::string>> methodResponse;
        methodResponseMsg.read(methodResponse);
        //TODO Check for empty response

        auto host = methodResponse.begin()->first;
        method = bus.new_method_call(
                    host.c_str(),
                    objpath.c_str(),
                    "org.freedesktop.DBus.Properties",
                    "Get");
        method.append(interf);
        //TODO Append 'tach' property

        methodResponseMsg = bus.call(method);
        methodResponseMsg.read(tach);

        return tach;
    }

} // namespace presence
} // namespace fan
} // namespace phosphor
