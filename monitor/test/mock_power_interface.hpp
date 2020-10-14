#pragma once

#include "../power_interface.hpp"

#include <gmock/gmock.h>

namespace phosphor::fan::monitor
{

class MockPowerInterface : public PowerInterfaceBase
{
  public:
    MOCK_METHOD(void, softPowerOff, (), (override));
    MOCK_METHOD(void, hardPowerOff, (), (override));
};

} // namespace phosphor::fan::monitor
