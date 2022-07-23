#pragma once

#include "sdbusplus.hpp"

#include <fmt/format.h>

#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/State/Host/server.hpp>

#include <functional>

using HostState =
    sdbusplus::xyz::openbmc_project::State::server::Host::HostState;

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
    PowerState(sdbusplus::bus_t& bus, StateChangeFunc callback) : _bus(bus)
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
    sdbusplus::bus_t& _bus;

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
    PGoodState(sdbusplus::bus_t& bus, StateChangeFunc func) :
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
    void pgoodChanged(sdbusplus::message_t& msg)
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
    sdbusplus::bus::match_t _match;
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
        PowerState(),
        _match(_bus,
               sdbusplus::bus::match::rules::propertiesChangedNamespace(
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
    HostPowerState(sdbusplus::bus_t& bus, StateChangeFunc func) :
        PowerState(bus, func),
        _match(_bus,
               sdbusplus::bus::match::rules::propertiesChangedNamespace(
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
    void hostStateChanged(sdbusplus::message_t& msg)
    {
        std::string interface;
        std::map<std::string, std::variant<std::string>> properties;
        std::vector<HostState> hostPowerStates;

        msg.read(interface, properties);

        auto hostStateProp = properties.find(_hostStateProperty);
        if (hostStateProp != properties.end())
        {
            auto currentHostState =
                sdbusplus::message::convert_from_string<HostState>(
                    std::get<std::string>(hostStateProp->second));

            if (!currentHostState)
            {
                throw sdbusplus::exception::InvalidEnumString();
            }
            HostState hostState = *currentHostState;

            hostPowerStates.emplace_back(hostState);
            setHostPowerState(hostPowerStates);
        }
    }

  private:
    void setHostPowerState(std::vector<HostState>& hostPowerStates)
    {
        bool powerStateflag = false;
        for (const auto& powerState : hostPowerStates)
        {
            if (powerState == HostState::Standby ||
                powerState == HostState::Running ||
                powerState == HostState::TransitioningToRunning ||
                powerState == HostState::Quiesced ||
                powerState == HostState::DiagnosticMode)
            {
                powerStateflag = true;
                break;
            }
        }
        setPowerState(powerStateflag);
    }

    /**
     * @brief Reads the CurrentHostState property from D-Bus and saves it.
     */
    void readHostState()
    {

        std::string hostStatePath;
        std::string hostStateService;
        std::string hostService = "xyz.openbmc_project.State.Host";
        std::vector<HostState> hostPowerStates;

        int32_t depth = 0;
        const std::string path = "/";

        auto mapperResponse = util::SDBusPlus::getSubTreeRaw(
            _bus, path, _hostStateInterface, depth);

        for (const auto& path : mapperResponse)
        {
            for (const auto& service : path.second)
            {
                hostStateService = service.first;

                if (hostStateService.find(hostService) != std::string::npos)
                {
                    hostStatePath = path.first;

                    auto currentHostState =
                        util::SDBusPlus::getProperty<HostState>(
                            hostStateService, hostStatePath,
                            _hostStateInterface, _hostStateProperty);

                    hostPowerStates.emplace_back(currentHostState);
                }
            }
        }
        setHostPowerState(hostPowerStates);
    }

    const std::string _hostStatePath{"/xyz/openbmc_project/state"};

    /** @brief D-Bus interface constant */
    const std::string _hostStateInterface{"xyz.openbmc_project.State.Host"};

    /** @brief D-Bus property constant */
    const std::string _hostStateProperty{"CurrentHostState"};

    /** @brief The propertiesChanged match */
    sdbusplus::bus::match_t _match;
};

} // namespace phosphor::fan
