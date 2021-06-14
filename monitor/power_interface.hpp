#pragma once

#include "types.hpp"

namespace phosphor::fan::monitor
{

/**
 * @class PowerInterfaceBase
 *
 * The base class that contains the APIs to do power offs.
 * This is required so it can be mocked in testcases.
 */
class PowerInterfaceBase
{
  public:
    PowerInterfaceBase() = default;
    virtual ~PowerInterfaceBase() = default;
    PowerInterfaceBase(const PowerInterfaceBase&) = delete;
    PowerInterfaceBase& operator=(const PowerInterfaceBase&) = delete;
    PowerInterfaceBase(PowerInterfaceBase&&) = delete;
    PowerInterfaceBase& operator=(PowerInterfaceBase&&) = delete;

    /**
     * @brief Perform a soft power off
     */
    virtual void softPowerOff() = 0;

    /**
     * @brief Perform a hard power off
     */
    virtual void hardPowerOff() = 0;

    /**
     * @brief Sets the thermal alert D-Bus property
     *
     * @param[in] alert - The alert value
     */
    virtual void thermalAlert(bool alert) = 0;
};

/**
 * @class PowerInterface
 *
 * Concrete class to perform power offs
 */
class PowerInterface : public PowerInterfaceBase
{
  public:
    PowerInterface() = delete;
    ~PowerInterface() = default;
    PowerInterface(const PowerInterface&) = delete;
    PowerInterface& operator=(const PowerInterface&) = delete;
    PowerInterface(PowerInterface&&) = delete;
    PowerInterface& operator=(PowerInterface&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] ThermalAlertObject& - The thermal alert D-Bus object
     */
    explicit PowerInterface(ThermalAlertObject& alertObject) :
        _alert(alertObject)
    {}

    /**
     * @brief Perform a soft power off
     */
    void softPowerOff() override;

    /**
     * @brief Perform a hard power off
     */
    void hardPowerOff() override;

    /**
     * @brief Sets the thermal alert D-Bus property
     *
     * @param[in] alert - The alert value
     */
    void thermalAlert(bool alert) override
    {
        _alert.enabled(alert);
    }

    /**
     * @brief Calls the D-Bus method to execute the hard power off.
     *
     * A static function so this can be used by code that doesn't
     * want to create a PowerInterface object.
     */
    static void executeHardPowerOff();

  private:
    /**
     * @brief Reference to the thermal alert D-Bus object
     */
    ThermalAlertObject& _alert;
};

} // namespace phosphor::fan::monitor
