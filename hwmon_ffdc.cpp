#include "hwmon_ffdc.hpp"

#include "logging.hpp"

#include <fmt/format.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace phosphor::fan::monitor
{

namespace util
{

namespace fs = std::filesystem;

inline std::vector<std::string> executeCommand(const std::string& command)
{
    std::vector<std::string> output;
    std::array<char, 128> buffer;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"),
                                                  pclose);
    if (!pipe)
    {
        getLogger().log(
            fmt::format("popen() failed when running command: {}", command));
        return output;
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        output.emplace_back(buffer.data());
    }

    return output;
}

std::vector<std::string> getHwmonNameFFDC()
{
    const fs::path hwmonBaseDir{"/sys/class/hwmon"};
    std::vector<std::string> hwmonNames;

    if (!fs::exists(hwmonBaseDir))
    {
        getLogger().log(fmt::format("Hwmon base directory {} doesn't exist",
                                    hwmonBaseDir.native()));
        return hwmonNames;
    }

    try
    {
        for (const auto& path : fs::directory_iterator(hwmonBaseDir))
        {
            if (!path.is_directory())
            {
                continue;
            }

            auto nameFile = path.path() / "name";
            if (fs::exists(nameFile))
            {
                std::ifstream f{nameFile};
                if (f.good())
                {
                    std::string name;
                    f >> name;
                    hwmonNames.push_back(name);
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        getLogger().log(
            fmt::format("Error traversing hwmon directories: {}", e.what()));
    }

    return hwmonNames;
}

std::vector<std::string> getDmesgFFDC()
{
    std::vector<std::string> output;
    auto dmesgOutput = executeCommand("dmesg");

    // Only pull in dmesg lines with interesting keywords.
    // One example is:
    // [   16.390603] max31785: probe of 7-0052 failed with error -110
    // using ' probe' to avoid 'modprobe'
    std::vector<std::string> matches{" probe", "failed"};

    for (const auto& line : dmesgOutput)
    {
        for (const auto& m : matches)
        {
            if (line.find(m) != std::string::npos)
            {
                output.push_back(line);
                if (output.back().back() == '\n')
                {
                    output.back().pop_back();
                }
                break;
            }
        }
    }

    return output;
}

} // namespace util

nlohmann::json collectHwmonFFDC()
{
    nlohmann::json ffdc;

    auto hwmonNames = util::getHwmonNameFFDC();
    if (!hwmonNames.empty())
    {
        ffdc["hwmonNames"] = std::move(hwmonNames);
    }

    auto dmesg = util::getDmesgFFDC();
    if (!dmesg.empty())
    {
        ffdc["dmesg"] = std::move(dmesg);
    }

    return ffdc;
}

} // namespace phosphor::fan::monitor
