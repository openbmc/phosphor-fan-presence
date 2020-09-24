#pragma once

#include "sdbusplus.hpp"

#include <functional>

namespace phosphor::fan
{

/**
 * @class PowerState
 *
 * This class provides an interface to check the current power state,
 * and to register a function that gets called when there is a power
 * state change.
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

    PowerState() = delete;
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
    PowerState(sdbusplus::bus::bus& bus, StateChangeFunc callback) :
        _bus(bus), _callback(std::move(callback))
    {}

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
     * Will call the callback function if the state changed.
     */
    void setPowerState(bool state)
    {
        if (state != _powerState)
        {
            _powerState = state;
            _callback(_powerState);
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
     * @brief The callback function to run when the power state changes
     */
    std::function<void(bool)> _callback;
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
    PGoodState() = delete;
    virtual ~PGoodState() = default;
    PGoodState(const PGoodState&) = delete;
    PGoodState& operator=(const PGoodState&) = delete;
    PGoodState(PGoodState&&) = delete;
    PGoodState& operator=(PGoodState&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] bus - The D-Bus bus connection object
     * @param[in] callback - The function that should be run when
     *                       the power state changes
     */
    PGoodState(sdbusplus::bus::bus& bus, StateChangeFunc func) :
        PowerState(bus, func)
    {
        using namespace sdbusplus::bus::match;

        _matches.emplace_back(
            _bus, rules::propertiesChanged(_pgoodPath, _pgoodInterface),
            [this](auto& msg) { this->pgoodChanged(msg); });

        _matches.emplace_back(
            _bus, rules::interfacesAdded() + rules::argNpath(0, _pgoodPath),
            [this](auto& msg) { this->pgoodInterfaceAdded(msg); });

        try
        {
            auto pgood = util::SDBusPlus::getProperty<int32_t>(
                _bus, _pgoodPath, _pgoodInterface, _pgoodProperty);

            _powerState = pgood;
        }
        catch (const util::DBusError& e)
        {
            // PGood service not available yet
        }
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

    /**
     * @brief Interfaces added callback for the PGOOD property.
     *
     * Will call the registered callback function if necessary.
     *
     * @param[in] msg - The payload of the interfacesAdded signal
     */
    void pgoodInterfaceAdded(sdbusplus::message::message& msg)
    {
        sdbusplus::message::object_path path;
        std::map<std::string, std::map<std::string, std::variant<int32_t>>>
            interfaces;

        msg.read(path, interfaces);

        auto interface = interfaces.find(_pgoodInterface);
        if (interface != interfaces.end())
        {
            auto prop = interface->second.find(_pgoodProperty);
            if (prop != interface->second.end())
            {
                setPowerState(std::get<int32_t>(prop->second));
            }
        }
    }

  private:
    /** @brief D-Bus path constant */
    const std::string _pgoodPath{"/org/openbmc/control/power0"};

    /** @brief D-Bus interface constant */
    const std::string _pgoodInterface{"org.openbmc.control.Power"};

    /** @brief D-Bus property constant */
    const std::string _pgoodProperty{"pgood"};

    /** @brief The interfacesAdded and propertiesChanged matches */
    std::vector<sdbusplus::bus::match::match> _matches;
};

} // namespace phosphor::fan
