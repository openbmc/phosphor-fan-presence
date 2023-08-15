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

#include "evdevpp/evdev.hpp"

#include <CLI/CLI.hpp>
// TODO https://github.com/openbmc/phosphor-fan-presence/issues/22
// #include "sdevent/event.hpp"
// #include "sdevent/io.hpp"
#include "utility.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <memory>

int main(int argc, char* argv[])
{
    CLI::App app{"evmon utility"};

    std::string path{};
    std::string stype{};
    std::string scode{};

    app.add_option("-p,--path", path, "evdev devpath")->required();
    app.add_option("-t,--type", stype, "evdev type")->required();
    app.add_option("-c,--code", scode, "evdev code")->required();

    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::Error& e)
    {
        return app.exit(e);
    }

    unsigned int type = EV_KEY;
    if (!stype.empty())
    {
        type = stoul(stype);
    }

    // TODO https://github.com/openbmc/phosphor-fan-presence/issues/22
    // auto loop = sdevent::event::newDefault();
    phosphor::fan::util::FileDescriptor fd(
        open(path.c_str(), O_RDONLY | O_NONBLOCK));
    auto evdev = evdevpp::evdev::newFromFD(fd());
    // sdevent::event::io::IO callback(loop, fd(), [&evdev](auto& s) {
    //     unsigned int type, code, value;
    //     std::tie(type, code, value) = evdev.next();
    //     std::cout << "type: " << libevdev_event_type_get_name(type)
    //               << " code: " << libevdev_event_code_get_name(type, code)
    //               << " value: " << value << "\n";
    // });

    auto value = evdev.fetch(type, stoul(scode));
    std::cout << "type: " << libevdev_event_type_get_name(type)
              << " code: " << libevdev_event_code_get_name(type, stoul(scode))
              << " value: " << value << "\n";

    // loop.loop();

    return 0;
}
