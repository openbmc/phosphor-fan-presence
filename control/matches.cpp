#include <sdbusplus/bus.hpp>
#include "matches.hpp"
#include "sdbusplus.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{
namespace match
{

using namespace phosphor::fan;
using namespace sdbusplus::bus::match;
using InternalFailure = sdbusplus::xyz::openbmc_project::Common::
                             Error::InternalFailure;

Match propertiesChanged(const std::string& obj, const std::string& iface)
{
    return [&obj, &iface]()
    {
        return rules::propertiesChanged(obj, iface);
    };
}

Match interfacesAdded(const std::string& obj)
{
    return [&obj]()
    {
        return rules::interfacesAdded(obj);
    };
}

Match nameOwnerChanged(const std::string& obj, const std::string& iface)
{
    return [&obj, &iface]()
    {
        std::string noc;
        try
        {
            noc = rules::nameOwnerChanged() +
                    rules::argN(0, util::SDBusPlus::getService(obj, iface));
        }
        catch (const InternalFailure& ife)
        {
            // Unable to construct NameOwnerChanged match string
        }
        return noc;
    };
}

} // namespace match
} // namespace control
} // namespace fan
} // namespace phosphor
