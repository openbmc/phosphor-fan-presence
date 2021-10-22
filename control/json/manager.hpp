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
#include "power_state.hpp"
#include "profile.hpp"
#include "sdbusplus.hpp"
#include "zone.hpp"

#include <fmt/format.h>

#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/event.hpp>
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
using namespace phosphor::logging;

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

/* Dbus signal object */
constexpr auto Path = 0;
constexpr auto Intf = 1;
constexpr auto Prop = 2;
using SignalObject = std::tuple<std::string, std::string, std::string>;
/* Dbus signal actions */
using SignalActions = std::vector<std::unique_ptr<ActionBase>>&;
/**
 * Signal handler function that handles parsing a signal's message for a
 * particular signal object and stores the results in the manager
 */
using SignalHandler = std::function<bool(sdbusplus::message::message&,
                                         const SignalObject&, Manager&)>;
/**
 * Package of data required when a signal is received
 * Tuple constructed of:
 *     SignalHandler = Signal handler function
 *     SignalObject = Dbus signal object
 *     SignalActions = List of actions that are run when the signal is received
 */
using SignalPkg = std::tuple<SignalHandler, SignalObject, SignalActions>;
/**
 * Data associated to a subscribed signal
 * Tuple constructed of:
 *     std::vector<SignalPkg> = List of the signal's packages
 *     std::unique_ptr<sdbusplus::server::match::match> =
 *         Pointer to match holding the subscription to a signal
 */
using SignalData = std::tuple<std::vector<SignalPkg>,
                              std::unique_ptr<sdbusplus::server::match::match>>;

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
     * @param[in] event - sdeventplus event loop
     */
    Manager(const sdeventplus::Event& event);

    /**
     * @brief Callback function to handle receiving a HUP signal to reload the
     * JSON configurations.
     */
    void sighupHandler(sdeventplus::source::Signal&,
                       const struct signalfd_siginfo*);

    /**
     * @brief Callback function to handle receiving a USR1 signal to dump
     * the flight recorder.
     */
    void sigUsr1Handler(sdeventplus::source::Signal&,
                        const struct signalfd_siginfo*);

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
     * @param[in] args - Arguments to be forwarded to each instance of `T`
     *   (*Note that a sdbusplus bus object is required as the first argument)
     *
     * @return Map of configuration entries
     *     Map of configuration keys to their corresponding configuration object
     */
    template <typename T, typename... Args>
    static std::map<configKey, std::unique_ptr<T>> getConfig(bool isOptional,
                                                             Args&&... args)
    {
        std::map<configKey, std::unique_ptr<T>> config;

        auto confFile =
            fan::JsonConfig::getConfFile(util::SDBusPlus::getBus(), confAppName,
                                         T::confFileName, isOptional);
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
            log<level::INFO>(
                fmt::format("Configuration({}) loaded successfully",
                            T::confFileName)
                    .c_str());
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
     * @brief Sets the dbus service owner state of a given object
     *
     * @param[in] path - Dbus object path
     * @param[in] serv - Dbus service name
     * @param[in] intf - Dbus object interface
     * @param[in] isOwned - Dbus service owner state
     */
    void setOwner(const std::string& path, const std::string& serv,
                  const std::string& intf, bool isOwned);

    /**
     * @brief Add a set of services for a path and interface by retrieving all
     * the path subtrees to the given depth from root for the interface
     *
     * @param[in] intf - Interface to add services for
     * @param[in] depth - Depth of tree traversal from root path
     *
     * @throws - DBusMethodError
     * Throws a DBusMethodError when the `getSubTree` method call fails
     */
    static void addServices(const std::string& intf, int32_t depth);

    /**
     * @brief Get the service for a given path and interface from cached
     * dataset and attempt to add all the services for the given path/interface
     * when it's not found
     *
     * @param[in] path - Path to get service for
     * @param[in] intf - Interface to get service for
     *
     * @return - The now cached service name
     *
     * @throws - DBusMethodError
     * Ripples up a DBusMethodError exception from calling addServices
     */
    static const std::string& getService(const std::string& path,
                                         const std::string& intf);

    /**
     * @brief Get all the object paths for a given service and interface from
     * the cached dataset and try to add all the services for the given
     * interface when no paths are found and then attempt to get all the object
     * paths again
     *
     * @param[in] serv - Service name to get paths for
     * @param[in] intf - Interface to get paths for
     *
     * @return The cached object paths
     */
    std::vector<std::string> getPaths(const std::string& serv,
                                      const std::string& intf);

    /**
     * @brief Add objects to the cached dataset by first using
     * `getManagedObjects` for the same service providing the given path and
     * interface or just add the single object of the given path, interface, and
     * property if that fails.
     *
     * @param[in] path - Dbus object's path
     * @param[in] intf - Dbus object's interface
     * @param[in] prop - Dbus object's property
     *
     * @throws - DBusMethodError
     * Throws a DBusMethodError when the the service is failed to be found or
     * when the `getManagedObjects` method call fails
     */
    void addObjects(const std::string& path, const std::string& intf,
                    const std::string& prop);

    /**
     * @brief Get an object's property value
     *
     * @param[in] path - Dbus object's path
     * @param[in] intf - Dbus object's interface
     * @param[in] prop - Dbus object's property
     */
    const std::optional<PropertyVariantType>
        getProperty(const std::string& path, const std::string& intf,
                    const std::string& prop);

    /**
     * @brief Set/update an object's property value
     *
     * @param[in] path - Dbus object's path
     * @param[in] intf - Dbus object's interface
     * @param[in] prop - Dbus object's property
     * @param[in] value - Dbus object's property value
     */
    void setProperty(const std::string& path, const std::string& intf,
                     const std::string& prop, PropertyVariantType value);

    /**
     * @brief Remove an object's interface
     *
     * @param[in] path - Dbus object's path
     * @param[in] intf - Dbus object's interface
     */
    inline void removeInterface(const std::string& path,
                                const std::string& intf)
    {
        auto itPath = _objects.find(path);
        if (itPath != std::end(_objects))
        {
            _objects[path].erase(intf);
        }
    }

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
     * @brief Get the signal data for a given match string
     *
     * @param[in] sigMatch - Signal match string
     *
     * @return - Reference to the signal data for the given match string
     */
    std::vector<SignalData>& getSignal(const std::string& sigMatch)
    {
        return _signals[sigMatch];
    }

    /**
     * @brief Handle receiving signals
     *
     * @param[in] msg - Signal message containing the signal's data
     * @param[in] pkgs - Signal packages associated to the signal being handled
     */
    void handleSignal(sdbusplus::message::message& msg,
                      const std::vector<SignalPkg>& pkgs);

    /**
     * @brief Get the sdbusplus bus object
     */
    inline auto& getBus()
    {
        return _bus;
    }

    /**
     * @brief Is the power state on
     *
     * @return Current power state of the system
     */
    inline bool isPowerOn() const
    {
        return _powerState->isPowerOn();
    }

    /**
     * @brief Load all the fan control JSON configuration files
     *
     * This is where all the fan control JSON configuration files are parsed and
     * loaded into their associated objects. Anything that needs to be done when
     * the Manager object is constructed or handling a SIGHUP to reload the
     * configurations needs to be done here.
     */
    void load();

    /**
     * @brief Sets a value in the parameter map.
     *
     * @param[in] name - The parameter name
     * @param[in] value - The parameter value
     */
    static void setParameter(const std::string& name,
                             const PropertyVariantType& value)
    {
        _parameters[name] = value;
    }

    /**
     * @brief Returns a value from the parameter map
     *
     * @param[in] name - The parameter name
     *
     * @return The parameter value, or std::nullopt if not found
     */
    static std::optional<PropertyVariantType>
        getParameter(const std::string& name)
    {
        auto it = _parameters.find(name);
        if (it != _parameters.end())
        {
            return it->second;
        }

        return std::nullopt;
    }

    /**
     * @brief Deletes a parameter from the parameter map
     *
     * @param[in] name - The parameter name
     */
    static void deleteParameter(const std::string& name)
    {
        _parameters.erase(name);
    }

  private:
    /**
     * @brief Helper to detect when a property's double contains a NaN
     * (not-a-number) value.
     *
     * @param[in] value - The property to test
     */
    static bool PropertyContainsNan(const PropertyVariantType& value)
    {
        return (std::holds_alternative<double>(value) &&
                std::isnan(std::get<double>(value)));
    }

    /* The sdbusplus bus object to use */
    sdbusplus::bus::bus& _bus;

    /* The sdeventplus even loop to use */
    sdeventplus::Event _event;

    /* The sdbusplus manager object to set the ObjectManager interface */
    sdbusplus::server::manager::manager _mgr;

    /* Whether loading the config files is allowed or not */
    bool _loadAllowed;

    /* The system's power state determination object */
    std::unique_ptr<PowerState> _powerState;

    /* List of profiles configured */
    std::map<configKey, std::unique_ptr<Profile>> _profiles;

    /* List of active profiles */
    static std::vector<std::string> _activeProfiles;

    /* Subtree map of paths to services of interfaces(with ownership state) */
    static std::map<
        std::string,
        std::map<std::string, std::pair<bool, std::vector<std::string>>>>
        _servTree;

    /* Object map of paths to interfaces of properties and their values */
    static std::map<
        std::string,
        std::map<std::string, std::map<std::string, PropertyVariantType>>>
        _objects;

    /* List of timers and their data to be processed when expired */
    std::vector<std::pair<std::unique_ptr<TimerData>, Timer>> _timers;

    /* Map of signal match strings to a list of signal handler data */
    std::unordered_map<std::string, std::vector<SignalData>> _signals;

    /* List of zones configured */
    std::map<configKey, std::unique_ptr<Zone>> _zones;

    /* List of events configured */
    std::map<configKey, std::unique_ptr<Event>> _events;

    /* The sdeventplus wrapper around sd_event_add_defer to dump the
     * flight recorder from the event loop after the USR1 signal.  */
    std::unique_ptr<sdeventplus::source::Defer> _flightRecEventSource;

    /**
     * @brief A map of parameter names and values that are something
     *        other than just D-Bus property values that other actions
     *        can set and use.
     */
    static std::unordered_map<std::string, PropertyVariantType> _parameters;

    /**
     * @brief Callback for power state changes
     *
     * @param[in] powerStateOn - Whether the power state is on or not
     *
     * Callback function bound to the PowerState object instance to handle each
     * time the power state changes.
     */
    void powerStateChanged(bool powerStateOn);

    /**
     * @brief Find the service name for a given path and interface from the
     * cached dataset
     *
     * @param[in] path - Path to get service for
     * @param[in] intf - Interface to get service for
     *
     * @return - The cached service name
     */
    static const std::string& findService(const std::string& path,
                                          const std::string& intf);

    /**
     * @brief Find all the paths for a given service and interface from the
     * cached dataset
     *
     * @param[in] serv - Service name to get paths for
     * @param[in] intf - Interface to get paths for
     *
     * @return - The cached object paths
     */
    std::vector<std::string> findPaths(const std::string& serv,
                                       const std::string& intf);

    /**
     * @brief Parse and set the configured profiles from the profiles JSON file
     *
     * Retrieves the optional profiles JSON configuration file, parses it, and
     * creates a list of configured profiles available to the other
     * configuration files. These profiles can be used to remove or include
     * entries within the other configuration files.
     */
    void setProfiles();

    /**
     * @brief Callback from _flightRecEventSource to dump the flight recorder
     */
    void dumpFlightRecorder(sdeventplus::source::EventBase&);
};

} // namespace phosphor::fan::control::json
