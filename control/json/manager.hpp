/**
 * Copyright © 2020 IBM Corporation
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
#pragma once

#include "action.hpp"
#include "config_base.hpp"
#include "event.hpp"
#include "group.hpp"
#include "json_config.hpp"
#include "profile.hpp"
#include "zone.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/timer.hpp>

#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

/* Application name to be appended to the path for loading a JSON config file */
constexpr auto confAppName = "control";

/* Type of timers supported */
enum class TimerType
{
    oneshot,
    repeating,
};
/**
 * Package of data required when a timer expires
 * Tuple constructed of:
 *      std::string = Timer package unique identifier
 *      std::vector<std::unique_ptr<ActionBase>> = List of pointers to actions
 * that run when the timer expires
 */
using TimerPkg =
    std::tuple<std::string, std::vector<std::unique_ptr<ActionBase>>&>;
/**
 * Data associated with a running timer that's used when it expires
 * Pair constructed of:
 *      TimerType = Type of timer to manage expired timer instances
 *      TimerPkg = Package of data required when the timer expires
 */
using TimerData = std::pair<TimerType, TimerPkg>;
/* Dbus event timer */
using Timer = sdeventplus::utility::Timer<sdeventplus::ClockId::Monotonic>;

/**
 * @class Manager - Represents the fan control manager's configuration
 *
 * A fan control manager configuration is optional, therefore the "manager.json"
 * file is also optional. The manager configuration is used to populate
 * fan control's manager parameters which are used in how the application
 * operates, not in how the fans are controlled.
 *
 * When no manager configuration exists, the fan control application starts,
 * processes any configured events and then begins controlling fans according
 * to those events.
 */
class Manager
{
  public:
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager& operator=(Manager&&) = delete;
    ~Manager() = default;

    /**
     * Constructor
     * Parses and populates the fan control manager attributes from a json file
     *
     * @param[in] bus - sdbusplus bus object
     * @param[in] event - sdeventplus event loop
     */
    Manager(sdbusplus::bus::bus& bus, const sdeventplus::Event& event);

    /**
     * @brief Get the active profiles of the system where an empty list
     * represents that only configuration entries without a profile defined will
     * be loaded.
     *
     * @return - The list of active profiles
     */
    static const std::vector<std::string>& getActiveProfiles();

    /**
     * @brief Load the configuration of a given JSON class object based on the
     * active profiles
     *
     * @param[in] isOptional - JSON configuration file is optional or not
     * @param[in] bus - The sdbusplus bus object
     * @param[in] args - Arguments to be forwarded to each instance of `T`
     *
     * @return Map of configuration entries
     *     Map of configuration keys to their corresponding configuration object
     */
    template <typename T, typename... Args>
    static std::map<configKey, std::unique_ptr<T>>
        getConfig(bool isOptional, sdbusplus::bus::bus& bus, Args&&... args)
    {
        std::map<configKey, std::unique_ptr<T>> config;

        auto confFile = fan::JsonConfig::getConfFile(
            bus, confAppName, T::confFileName, isOptional);
        if (!confFile.empty())
        {
            for (const auto& entry : fan::JsonConfig::load(confFile))
            {
                if (entry.contains("profiles"))
                {
                    std::vector<std::string> profiles;
                    for (const auto& profile : entry["profiles"])
                    {
                        profiles.emplace_back(
                            profile.template get<std::string>());
                    }
                    // Do not create the object if its profiles are not in the
                    // list of active profiles
                    if (!profiles.empty() &&
                        !std::any_of(profiles.begin(), profiles.end(),
                                     [](const auto& name) {
                                         return std::find(
                                                    getActiveProfiles().begin(),
                                                    getActiveProfiles().end(),
                                                    name) !=
                                                getActiveProfiles().end();
                                     }))
                    {
                        continue;
                    }
                }
                auto obj =
                    std::make_unique<T>(entry, std::forward<Args>(args)...);
                config.emplace(
                    std::make_pair(obj->getName(), obj->getProfiles()),
                    std::move(obj));
            }
        }
        return config;
    }

    /**
     * @brief Check if the given input configuration key matches with another
     * configuration key that it's to be included in
     *
     * @param[in] input - Config key to be included in another config object
     * @param[in] comp - Config key of the config object to compare with
     *
     * @return Whether the configuration object should be included
     */
    static bool inConfig(const configKey& input, const configKey& comp);

    /**
     * @brief Check if the given path and inteface is owned by a dbus service
     *
     * @param[in] path - Dbus object path
     * @param[in] intf - Dbus object interface
     *
     * @return - Whether the service has an owner for the given object path and
     * interface
     */
    static bool hasOwner(const std::string& path, const std::string& intf);

    /**
     * @brief Get the object's property value as a variant
     *
     * @param[in] path - Path of the object containing the property
     * @param[in] intf - Interface name containing the property
     * @param[in] prop - Name of property
     *
     * @return - The object's property value as a variant
     */
    static inline auto getObjValueVariant(const std::string& path,
                                          const std::string& intf,
                                          const std::string& prop)
    {
        return _objects.at(path).at(intf).at(prop);
    };

    /**
     * @brief Add a dbus timer
     *
     * @param[in] type - Type of timer
     * @param[in] interval - Timer interval in microseconds
     * @param[in] pkg - Packaged data for when timer expires
     */
    void addTimer(const TimerType type,
                  const std::chrono::microseconds interval,
                  std::unique_ptr<TimerPkg> pkg);

    /**
     * @brief Callback when a timer expires
     *
     * @param[in] data - Data to be used when the timer expired
     */
    void timerExpired(TimerData& data);

    /**
     * @brief Get the configured power on delay(OPTIONAL)
     *
     * @return Power on delay in seconds
     *     Configured power on delay in seconds, otherwise 0
     */
    unsigned int getPowerOnDelay();

  private:
    /* JSON file name for manager configuration attributes */
    static constexpr auto confFileName = "manager.json";

    /* The parsed JSON object */
    json _jsonObj;

    /**
     * The sdbusplus bus object to use
     */
    sdbusplus::bus::bus& _bus;

    /**
     * The sdeventplus even loop to use
     */
    sdeventplus::Event _event;

    /* List of profiles configured */
    std::map<configKey, std::unique_ptr<Profile>> _profiles;

    /* List of active profiles */
    static std::vector<std::string> _activeProfiles;

    /* Subtree map of paths to services (with ownership state) of interfaces */
    static std::map<std::string, std::map<std::pair<std::string, bool>,
                                          std::vector<std::string>>>
        _servTree;

    /* Object map of paths to interfaces of properties and their values */
    static std::map<
        std::string,
        std::map<std::string, std::map<std::string, PropertyVariantType>>>
        _objects;

    /* List of timers and their data to be processed when expired */
    std::vector<std::pair<std::unique_ptr<TimerData>, Timer>> _timers;

    /* List of zones configured */
    std::map<configKey, std::unique_ptr<Zone>> _zones;

    /* List of events configured */
    std::map<configKey, std::unique_ptr<Event>> _events;

    /**
     * @brief Parse and set the configured profiles from the profiles JSON file
     *
     * Retrieves the optional profiles JSON configuration file, parses it, and
     * creates a list of configured profiles available to the other
     * configuration files. These profiles can be used to remove or include
     * entries within the other configuration files.
     */
    void setProfiles();
};

} // namespace phosphor::fan::control::json
