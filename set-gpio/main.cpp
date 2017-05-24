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

/**
 * This program is a utility for setting GPIOs.
 * Modes:
 *   low:  Set a GPIO low
 *   high: Set a GPIO high
 *   low_high: Set a GPIO low, delay if requested, set it high
 *   high_low: Set a GPIO high, delay if requested, set it low
 */

#include <algorithm>
#include <map>
#include <iostream>
#include <phosphor-logging/log.hpp>
#include "argument.hpp"
#include "gpio.hpp"

using namespace phosphor::fan::util;
using namespace phosphor::gpio;
using namespace phosphor::logging;

typedef void (*gpioFunction)(GPIO&, unsigned int);


/**
 * Sets a GPIO low
 *
 * @param[in] gpio - the GPIO object
 * @param[in] delayInMS - Unused in this function
 */
static void low(GPIO& gpio,
                unsigned int delayInMS)
{
    gpio.setLow();
}


/**
 * Sets a GPIO high
 *
 * @param[in] gpio - the GPIO object
 * @param[in] delayInMS - Unused in this function
 */
static void high(GPIO& gpio,
                 unsigned int delayInMS)
{
    gpio.setHigh();
}


/**
 * Sets a GPIO high, then delays, then sets it low
 *
 * @param[in] gpio - the GPIO object
 * @param[in] delayInMS - The delay in between the sets
 */
static void highLow(GPIO& gpio,
                    unsigned int delayInMS)
{
    gpio.setHigh();

    usleep(delayInMS * 1000);

    gpio.setLow();
}


/**
 * Sets a GPIO low, then delays, then sets it high
 *
 * @param[in] gpio - the GPIO to write
 * @param[in] delayInMS - The delay in between the sets
 */
static void lowHigh(GPIO& gpio,
                    unsigned int delayInMS)
{
    gpio.setLow();

    usleep(delayInMS * 1000);

    gpio.setHigh();
}


using gpioFunctionMap = std::map<std::string, gpioFunction>;

static const gpioFunctionMap functions
{
    {"low", low},
    {"high", high},
    {"low_high", lowHigh},
    {"high_low", highLow}
};

/**
 * Prints usage and exits the program
 *
 * @param[in] err - the error message to print
 * @param[in] argv - argv from main()
 */
static void exitWithError(const char* err, char** argv)
{
    std::cerr << "ERROR: " << err << "\n";
    ArgumentParser::usage(argv);

    std::cerr << "Valid modes:\n";
    std::for_each(functions.begin(), functions.end(),
                  [](const auto& f)
                  {
                      std::cerr << "  " << f.first << "\n";
                  });
    exit(EXIT_FAILURE);
}


/**
 * Returns the unsigned int value of the argument passed in
 *
 * @param[in] name - the argument name
 * @param[in] parser - the argument parser
 * @param[in] argv - arv from main()
 */
static unsigned int getUintFromArg(const char* name,
                                   ArgumentParser& parser,
                                   char** argv)
{
    char* p = NULL;
    auto val = strtol(parser[name].c_str(), &p, 10);

    //strol sets p on error, also we don't allow negative values
    if (*p || (val < 0))
    {
        using namespace std::string_literals;
        std::string msg = "Invalid "s + name + " value passed in";
        exitWithError(msg.c_str(), argv);
    }
    return static_cast<unsigned int>(val);
}


int main(int argc, char** argv)
{
    ArgumentParser args(argc, argv);

    auto device = args["device"];
    if (device == ArgumentParser::empty_string)
    {
        exitWithError("GPIO device not specified", argv);
    }

    auto mode = args["mode"];
    if (mode == ArgumentParser::empty_string)
    {
        exitWithError("Mode not specified", argv);
    }

    if (args["gpio"] == ArgumentParser::empty_string)
    {
        exitWithError("GPIO not specified", argv);
    }

    auto gpioNum = getUintFromArg("gpio", args, argv);

    //Not all use modes require a delay, so not required
    unsigned int delay = 0;
    if (args["delay"] != ArgumentParser::empty_string)
    {
        delay = getUintFromArg("delay", args, argv);
    }

    auto function = functions.find(mode);
    if (function == functions.end())
    {
        exitWithError("Invalid mode value passed in", argv);
    }

    GPIO gpio{device, gpioNum, GPIO::Direction::output};

    try
    {
        function->second(gpio, delay);
    }
    catch (std::runtime_error& e)
    {
        return -1;
    }

    return 0;
}
