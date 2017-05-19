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
#include <sdbusplus/bus.hpp>
#include "argument.hpp"
#include "manager.hpp"

using namespace phosphor::fan::control;

int main(int argc, char* argv[])
{
    auto bus = sdbusplus::bus::new_default();
    phosphor::fan::util::ArgumentParser args(argc, argv);

    if (argc != 2)
    {
        args.usage(argv);
        exit(-1);
    }

    Mode mode;

    if (args["init"] == "true")
    {
        mode = Mode::init;
    }
    else if (args["control"] == "true")
    {
        mode = Mode::control;
    }
    else
    {
        args.usage(argv);
        exit(-1);
    }

    Manager manager(bus, mode);

    //Init mode will just set fans to max and delay
    if (mode == Mode::init)
    {
        manager.doInit();
        return 0;
    }

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }

    return 0;
}
