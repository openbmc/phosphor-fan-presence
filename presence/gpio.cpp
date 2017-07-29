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
#include <memory>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <tuple>
#include <xyz/openbmc_project/Common/Callout/error.hpp>
#include "gpio.hpp"
#include "rpolicy.hpp"
#include "sdevent.hpp"

namespace phosphor
{
namespace fan
{
namespace presence
{

Gpio::Gpio(
        const std::string& physDevice,
        const std::string& device,
        unsigned int physPin) :
    currentState(false),
    evdevfd(open(device.c_str(), O_RDONLY | O_NONBLOCK)),
    evdev(evdevpp::evdev::newFromFD(evdevfd())),
    phys(physDevice),
    pin(physPin),
    callback(nullptr)
{

}

bool Gpio::start()
{
    callback = std::make_unique<sdevent::event::io::IO>(
            util::SDEvent::getEvent(),
            evdevfd(),
            [this](auto& s){this->ioCallback(s);});
    currentState = present();
    return currentState;
}

void Gpio::stop()
{
    callback = nullptr;
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
            GPIO::CALLOUT_GPIO_NUM(pin),
            GPIO::CALLOUT_ERRNO(0),
            GPIO::CALLOUT_DEVICE_PATH(phys.c_str()));
}

void Gpio::ioCallback(sdevent::source::Source& source)
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
} // namespace presence
} // namespace fan
} // namespace phosphor
