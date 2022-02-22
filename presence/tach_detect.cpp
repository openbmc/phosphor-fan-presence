/**
 * Copyright © 2022 IBM Corporation
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
#include "config.h"
#ifdef PRESENCE_USE_JSON
#include "json_config.hpp"
#include "json_parser.hpp"
#include "utility.hpp"
#else
#include "generated.hpp"
#endif
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>
#include <stdplus/signal.hpp>

#include <functional>
#include <iostream>

namespace presence = phosphor::fan::presence;

namespace match = sdbusplus::bus::match;

using namespace phosphor::fan;

using Match = sdbusplus::bus::match_t;

struct TachPresence
{
    TachPresence(sdeventplus::Event &event);

protected:

    void setup();

#ifdef PRESENCE_USE_JSON

    void checkCompatIntfAndContinue();

    void checkInventoryAndContinue();

    void compatIntfAdded(sdbusplus::message::message&);

    void loadJsonAndStart();

    sdeventplus::Event & _event;

    sdbusplus::bus::bus _bus;

    std::unique_ptr<Match> _compatibleMatch, _inventoryMatch;

    std::unique_ptr<presence::JsonConfig> _presenceConfig;

    std::unique_ptr<phosphor::fan::JsonConfig> _jsonConfig;

#endif

};


#ifdef PRESENCE_USE_JSON

void TachPresence::checkInventoryAndContinue()
{
    bool invServiceRunning = util::SDBusPlus::callMethodAndRead<bool>(
        _bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
        "org.freedesktop.DBus", "NameHasOwner", util::INVENTORY_SVC);

    if (invServiceRunning)
    {
        // inventory is running, now check compatible interface
        checkCompatIntfAndContinue();
    }
    else
    {
        // wait for inventory to appear
        _inventoryMatch = std::make_unique<Match>(
            _bus, match::rules::nameOwnerChanged(util::INVENTORY_SVC),
            [=](auto& msg) {
                std::string msgStr;
                msg.read(msgStr);

                // first string is inteface, make sure it's for us
                if (util::INVENTORY_INTF != msgStr)
                {
                    return;
                }

                msg.read(msgStr); // old name, not used
                msg.read(msgStr); // new name

                if (!msgStr.empty())
                {
                    checkCompatIntfAndContinue();
                }
            });
    }
}

void TachPresence::checkCompatIntfAndContinue()
{
    bool compatServiceRunning = util::SDBusPlus::callMethodAndRead<bool>(
        _bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
        "org.freedesktop.DBus", "NameHasOwner", confCompatServ);

    if(compatServiceRunning)
    {
std::cout << "Tracer: " << __LINE__ << std::endl;
        loadJsonAndStart();

    } else {

        // TODO: this probably is wrong to begin with
std::cout << "Tracer: " << __LINE__ << std::endl;
        _compatibleMatch = std::make_unique<Match>(_bus,
            match::rules::interfacesAdded() +
                match::rules::sender(confCompatServ),
            std::bind(&TachPresence::compatIntfAdded, this, std::placeholders::_1));
    }
}


void TachPresence::compatIntfAdded(sdbusplus::message::message& msg)
{
    sdbusplus::message::object_path op;

    std::map<std::string,
             std::map<std::string, std::variant<std::vector<std::string>>>>
        intfProps;

    msg.read(op, intfProps);

    if (intfProps.find(confCompatIntf) == intfProps.end())
    {
        return;
    }

    const auto& props = intfProps.at(confCompatIntf);

    // Only one dbus object with the compatible interface is used at a time
    auto _confCompatValues =
        std::get<std::vector<std::string>>(props.at(confCompatProp));

    if(_confCompatValues.size() > 0)
    {
std::cout << "Tracer: " << __LINE__ << std::endl;
        loadJsonAndStart();
    }
}

void TachPresence::loadJsonAndStart()
{
    // prevent any more triggers
    _compatibleMatch.reset();
    _inventoryMatch.reset();

    _presenceConfig = std::make_unique<presence::JsonConfig>(_bus);

    // jsonConfig will call config::start when
    // the conf file is available.
    _jsonConfig = std::make_unique<phosphor::fan::JsonConfig>(
        std::bind(&presence::JsonConfig::start, _presenceConfig.get()));

    // TODO: make this a uniqe_ptr
    sdeventplus::source::Signal signal(
        _event, SIGHUP,
        std::bind(&presence::JsonConfig::sighupHandler, _presenceConfig.get(),
                  std::placeholders::_1, std::placeholders::_2));
}

void TachPresence::setup()
{
    stdplus::signal::block(SIGHUP);

    checkInventoryAndContinue();
}

TachPresence::TachPresence(sdeventplus::Event &event) :
    _event { event },
    _bus {sdbusplus::bus::new_default()}
{
    _bus.attach_event(event.get(), SD_EVENT_PRIORITY_NORMAL);

    setup();
}

#else

void TachPresence::setup()
{
    for (auto& p : presence::ConfigPolicy::get())
    {
        p->monitor();
    }
}

TachPresence::TachPresence(sdeventplus::Event &event) :
{
    setup();
}

#endif

int main(void)
{
    auto event = sdeventplus::Event::get_default();

    TachPresence tachPresence(event);

    return event.loop();
}
