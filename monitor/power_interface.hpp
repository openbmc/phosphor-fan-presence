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
    PowerInterface(const PowerInterface&) = delete;
    PowerInterface& operator=(const PowerInterface&) = delete;
    PowerInterface(PowerInterface&&) = delete;
    PowerInterface& operator=(PowerInterface&&) = delete;

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
