#pragma once

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
    PowerInterfaceBase(const PowerInterfaceBase&) = default;
    PowerInterfaceBase& operator=(const PowerInterfaceBase&) = default;
    PowerInterfaceBase(PowerInterfaceBase&&) = default;
    PowerInterfaceBase& operator=(PowerInterfaceBase&&) = default;

    /**
     * @brief Perform a soft power off
     */
    virtual void softPowerOff() = 0;

    /**
     * @brief Perform a hard power off
     */
    virtual void hardPowerOff() = 0;
};

/**
 * @class PowerInterface
 *
 * Concrete class to perform power offs
 */
class PowerInterface : public PowerInterfaceBase
{
  public:
    PowerInterface() = default;
    ~PowerInterface() = default;
    PowerInterface(const PowerInterface&) = default;
    PowerInterface& operator=(const PowerInterface&) = default;
    PowerInterface(PowerInterface&&) = default;
    PowerInterface& operator=(PowerInterface&&) = default;

    /**
     * @brief Perform a soft power off
     */
    void softPowerOff() override;

    /**
     * @brief Perform a hard power off
     */
    void hardPowerOff() override;
};

} // namespace phosphor::fan::monitor
