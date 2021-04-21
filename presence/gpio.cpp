/**
 * Copyright © 2017 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "gpio.hpp"

#include "logging.hpp"
#include "rpolicy.hpp"
#include "sdbusplus.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <sdeventplus/event.hpp>
#include <xyz/openbmc_project/Common/Callout/error.hpp>
#include <xyz/openbmc_project/Logging/Entry/server.hpp>

#include <functional>
#include <tuple>

namespace phosphor
{
namespace fan
{
namespace presence
{

const auto loggingService = "xyz.openbmc_project.Logging";
const auto loggingPath = "/xyz/openbmc_project/logging";
const auto loggingCreateIface = "xyz.openbmc_project.Logging.Create";

Gpio::Gpio(const std::string& physDevice, const std::string& device,
           unsigned int physPin) :
    currentState(false),
    evdevfd(open(device.c_str(), O_RDONLY | O_NONBLOCK)),
    evdev(evdevpp::evdev::newFromFD(evdevfd())), phys(physDevice), pin(physPin)
{}

bool Gpio::start()
{
    source.emplace(sdeventplus::Event::get_default(), evdevfd(), EPOLLIN,
                   std::bind(&Gpio::ioCallback, this));
    currentState = present();
    return currentState;
}

void Gpio::stop()
{
    source.reset();
}

bool Gpio::present()
{
    return evdev.fetch(EV_KEY, pin) != 0;
}

void Gpio::fail()
{
    using namespace sdbusplus::xyz::openbmc_project::Common::Callout::Error;
    using namespace phosphor::logging;
    using namespace xyz::openbmc_project::Common::Callout;

    report<sdbusplus::xyz::openbmc_project::Common::Callout::Error::GPIO>(
        GPIO::CALLOUT_GPIO_NUM(pin), GPIO::CALLOUT_ERRNO(0),
        GPIO::CALLOUT_DEVICE_PATH(phys.c_str()));
}

void Gpio::ioCallback()
{
    unsigned int type, code, value;

    std::tie(type, code, value) = evdev.next();
    if (type != EV_KEY || code != pin)
    {
        return;
    }

    bool newState = value != 0;

    if (currentState != newState)
    {
        getPolicy().stateChanged(newState, *this);
        currentState = newState;
    }
}

void Gpio::logConflict(const std::string& fanInventoryPath) const
{
    using namespace sdbusplus::xyz::openbmc_project::Logging::server;
    std::map<std::string, std::string> ad;
    Entry::Level severity = Entry::Level::Informational;

    static constexpr auto errorName =
        "xyz.openbmc_project.Fan.Presence.Error.Detection";

    ad.emplace("_PID", std::to_string(getpid()));
    ad.emplace("CALLOUT_INVENTORY_PATH", fanInventoryPath);
    ad.emplace("GPIO_NUM", std::to_string(pin));
    ad.emplace("GPIO_DEVICE_PATH", (phys.c_str()));

    getLogger().log(
        fmt::format("GPIO presence detect for fan {} said not present but "
                    "other methods indicated present",
                    fanInventoryPath));
    try
    {
        util::SDBusPlus::callMethod(loggingService, loggingPath,
                                    loggingCreateIface, "Create", errorName,
                                    severity, ad);
    }
    catch (const util::DBusError& e)
    {
        getLogger().log(
            fmt::format("Call to create a {} error for fan {} failed: {}",
                        errorName, fanInventoryPath, e.what()),
            Logger::error);
    }
}

} // namespace presence
} // namespace fan
} // namespace phosphor
