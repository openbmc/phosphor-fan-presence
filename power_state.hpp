#pragma once

#include "sdbusplus.hpp"

#include <fmt/format.h>

#include <boost/asio/io_service.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <functional>
#include <iostream>

constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_OBJ_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_IFACE = "xyz.openbmc_project.ObjectMapper";

using Service = std::string;
using Path = std::string;
using Interface = std::string;
using Interfaces = std::vector<Interface>;
using MapperResponseType = std::map<Path, std::map<Service, Interfaces>>;

const std::string HOST_IFACE = "xyz.openbmc_project.State.Host";

namespace phosphor::fan
{

/**
 * @class PowerState
 *
 * This class provides an interface to check the current power state,
 * and to register a function that gets called when there is a power
 * state change.  A callback can be passed in using the constructor,
 * or can be added later using addCallback().
 *
 * Different architectures may have different ways of considering
 * power to be on, such as a pgood property on the
 * org.openbmc.Control.Power interface, or the CurrentPowerState
 * property on the State.Chassis interface, so those details will
 * be in a derived class.
 */
class PowerState
{
  public:
    using StateChangeFunc = std::function<void(bool)>;

    virtual ~PowerState() = default;
    PowerState(const PowerState&) = delete;
    PowerState& operator=(const PowerState&) = delete;
    PowerState(PowerState&&) = delete;
    PowerState& operator=(PowerState&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] bus - The D-Bus bus connection object
     * @param[in] callback - The function that should be run when
     *                       the power state changes
     */
    PowerState(sdbusplus::bus::bus& bus, StateChangeFunc callback) : _bus(bus)
    {
        _callbacks.emplace("default", std::move(callback));
    }

    /**
     * @brief Constructor
     *
     * Callbacks can be added with addCallback().
     */
    PowerState() : _bus(util::SDBusPlus::getBus())
    {}

    /**
     * @brief Adds a function to call when the power state changes
     *
     * @param[in] - Any unique name, so the callback can be removed later
     *              if desired.
     * @param[in] callback - The function that should be run when
     *                       the power state changes
     */
    void addCallback(const std::string& name, StateChangeFunc callback)
    {
        _callbacks.emplace(name, std::move(callback));
    }

    /**
     * @brief Remove the callback so it is no longer called
     *
     * @param[in] name - The name used when it was added.
     */
    void deleteCallback(const std::string& name)
    {
        _callbacks.erase(name);
    }

    /**
     * @brief Says if power is on
     *
     * @return bool - The power state
     */
    bool isPowerOn() const
    {
        return _powerState;
    }

  protected:
    /**
     * @brief Called by derived classes to set the power state value
     *
     * Will call the callback functions if the state changed.
     *
     * @param[in] state - The new power state
     */
    void setPowerState(bool state)
    {
        if (state != _powerState)
        {
            _powerState = state;
            for (const auto& [name, callback] : _callbacks)
            {
                callback(_powerState);
            }
        }
    }

    /**
     * @brief Reference to the D-Bus connection object.
     */
    sdbusplus::bus::bus& _bus;

    /**
     * @brief The power state value
     */
    bool _powerState = false;

  private:
    /**
     * @brief The callback functions to run when the power state changes
     */
    std::map<std::string, StateChangeFunc> _callbacks;
};

/**
 * @class PGoodState
 *
 * This class implements the PowerState API by looking at the 'pgood'
 * property on the org.openbmc.Control.Power interface.
 */
class PGoodState : public PowerState
{
  public:
    virtual ~PGoodState() = default;
    PGoodState(const PGoodState&) = delete;
    PGoodState& operator=(const PGoodState&) = delete;
    PGoodState(PGoodState&&) = delete;
    PGoodState& operator=(PGoodState&&) = delete;

    PGoodState() :
        PowerState(), _match(_bus,
                             sdbusplus::bus::match::rules::propertiesChanged(
                                 _pgoodPath, _pgoodInterface),
                             [this](auto& msg) { this->pgoodChanged(msg); })
    {
        readPGood();
    }

    /**
     * @brief Constructor
     *
     * @param[in] bus - The D-Bus bus connection object
     * @param[in] callback - The function that should be run when
     *                       the power state changes
     */
    PGoodState(sdbusplus::bus::bus& bus, StateChangeFunc func) :
        PowerState(bus, func),
        _match(_bus,
               sdbusplus::bus::match::rules::propertiesChanged(_pgoodPath,
                                                               _pgoodInterface),
               [this](auto& msg) { this->pgoodChanged(msg); })
    {
        readPGood();
    }

    /**
     * @brief PropertiesChanged callback for the PGOOD property.
     *
     * Will call the registered callback function if necessary.
     *
     * @param[in] msg - The payload of the propertiesChanged signal
     */
    void pgoodChanged(sdbusplus::message::message& msg)
    {
        std::string interface;
        std::map<std::string, std::variant<int32_t>> properties;

        msg.read(interface, properties);

        auto pgoodProp = properties.find(_pgoodProperty);
        if (pgoodProp != properties.end())
        {
            auto pgood = std::get<int32_t>(pgoodProp->second);
            setPowerState(pgood);
        }
    }

  private:
    /**
     * @brief Reads the PGOOD property from D-Bus and saves it.
     */
    void readPGood()
    {
        try
        {
            auto pgood = util::SDBusPlus::getProperty<int32_t>(
                _bus, _pgoodPath, _pgoodInterface, _pgoodProperty);

            _powerState = static_cast<bool>(pgood);
        }
        catch (const util::DBusServiceError& e)
        {
            // Wait for propertiesChanged signal when service starts
        }
    }

    /** @brief D-Bus path constant */
    const std::string _pgoodPath{"/org/openbmc/control/power0"};

    /** @brief D-Bus interface constant */
    const std::string _pgoodInterface{"org.openbmc.control.Power"};

    /** @brief D-Bus property constant */
    const std::string _pgoodProperty{"pgood"};

    /** @brief The propertiesChanged match */
    sdbusplus::bus::match::match _match;
};

/**
 * @class HostPowerState
 *
 * This class implements the PowerState API by looking at the 'powerState'
 * property on the phosphor virtual sensor interface.
 */
class HostPowerState : public PowerState
{
  public:
    virtual ~HostPowerState() = default;
    HostPowerState(const HostPowerState&) = delete;
    HostPowerState& operator=(const HostPowerState&) = delete;
    HostPowerState(HostPowerState&&) = delete;
    HostPowerState& operator=(HostPowerState&&) = delete;

    HostPowerState() :
        PowerState(), _match(_bus,
                             sdbusplus::bus::match::rules::propertiesChanged(
                                 _hostStatePath, _hostStateInterface),
                             [this](auto& msg) { this->hostStateChanged(msg); })
    {
        readHostState();
    }

    /**
     * @brief Constructor
     *
     * @param[in] bus - The D-Bus bus connection object
     * @param[in] callback - The function that should be run when
     *                       the power state changes
     */
    HostPowerState(sdbusplus::bus::bus& bus, StateChangeFunc func) :
        PowerState(bus, func),
        _match(_bus,
               sdbusplus::bus::match::rules::propertiesChanged(
                   _hostStatePath, _hostStateInterface),
               [this](auto& msg) { this->hostStateChanged(msg); })
    {
        readHostState();
    }

    /**
     * @brief PropertiesChanged callback for the CurrentHostState property.
     *
     * Will call the registered callback function if necessary.
     *
     * @param[in] msg - The payload of the propertiesChanged signal
     */
    void hostStateChanged(sdbusplus::message::message& msg)
    {
        std::string interface;
        std::map<std::string, std::variant<std::string>> properties;

        msg.read(interface, properties);

        auto hostStateProp = properties.find(_hostStateProperty);
        if (hostStateProp != properties.end())
        {
            std::string hostState =
                std::get<std::string>(hostStateProp->second);
            std::string last_element(
                hostState.substr(hostState.rfind(".") + 1));

            if (last_element == "Running")
            {
                setPowerState(true);
            }
            if (last_element == "Off")
            {
                setPowerState(false);
            }
        }
    }

    void getChassisSubTree(sdbusplus::bus::bus& bus,
                           MapperResponseType& subtree)
    {

        int32_t depth = 0;
        const std::string path = "/";
        std::vector<std::string> hostIntfs = {HOST_IFACE};

        auto mapperCall = bus.new_method_call(MAPPER_BUSNAME, MAPPER_OBJ_PATH,
                                              MAPPER_IFACE, "GetSubTree");

        mapperCall.append(path);
        mapperCall.append(depth);
        mapperCall.append(hostIntfs);

        try
        {
            auto mapperResponseMsg = bus.call(mapperCall);
            mapperResponseMsg.read(subtree);
        }
        catch (const sdbusplus::exception::exception& e)
        {
            std::cerr << "Chassis GetSubTree read failed : " << e.what()
                      << std::endl;
        }
    }

  private:
    /**
     * @brief Reads the CurrentHostState property from D-Bus and saves it.
     */
    void readHostState()
    {
        std::string hostStateservice;
        std::string hostService = "xyz.openbmc_project.State.Host";

        MapperResponseType mapperResponse;
        getChassisSubTree(_bus, mapperResponse);

        if (mapperResponse.empty())
        {
            // No errors to process.
            return;
        }

        for (const auto& elem : mapperResponse)
        {
            for (auto serviceMap : elem.second)
            {
                hostStateservice = serviceMap.first.c_str();
                if (hostStateservice.find(hostService) != std::string::npos)
                {
                    _hostStatePath = elem.first.c_str();

                    try
                    {
                        auto CurrentHostState =
                            util::SDBusPlus::getProperty<std::string>(
                                hostStateservice, _hostStatePath,
                                _hostStateInterface, _hostStateProperty);

                        std::string last_element(CurrentHostState.substr(
                            CurrentHostState.rfind(".") + 1));

                        if (last_element == "Running")
                        {
                            setPowerState(true);
                        }
                        if (last_element == "Off")
                        {
                            setPowerState(false);
                        }
                    }
                    catch (const util::DBusServiceError& e)
                    {
                        // Wait.. for propertiesChanged signal when service
                        // starts
                    }
                }
            }
        }
    }

    /** @brief D-Bus path constant */
    std::string _hostStatePath;

    /** @brief D-Bus interface constant */
    const std::string _hostStateInterface{"xyz.openbmc_project.State.Host"};

    /** @brief D-Bus property constant */
    const std::string _hostStateProperty{"CurrentHostState"};

    /** @brief The propertiesChanged match */
    sdbusplus::bus::match::match _match;
};

} // namespace phosphor::fan
