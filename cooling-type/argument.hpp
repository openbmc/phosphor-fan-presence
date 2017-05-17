#pragma once

#include <getopt.h>
#include <map>
#include <string>

namespace phosphor
{
namespace cooling
{
namespace type
{

class ArgumentParser
{
    public:
        ArgumentParser(int argc, char** argv);
        ArgumentParser() = delete;
        ArgumentParser(const ArgumentParser&) = delete;
        ArgumentParser(ArgumentParser&&) = default;
        ArgumentParser& operator=(const ArgumentParser&) = delete;
        ArgumentParser& operator=(ArgumentParser&&) = default;
        ~ArgumentParser() = default;
        const std::string& operator[](const std::string& opt);

        static void usage(char** argv);

        static constexpr auto true_string = "true";
        static const std::string empty_string;

    private:
        std::map<const std::string, std::string> arguments;

        static const option options[];
        static const char* optionstr;
};

}
}
}
// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
