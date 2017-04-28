/* This is a generated file. */
#include "fan_defs.hpp"
#include "types.hpp"

using namespace phosphor::fan::monitor;

const std::vector<FanDefinition> fanDefinitions
{
    FanDefinition{"/system/chassis/motherboard/fan0",
                  30,
                  15,
                  15,
                  1,
                  std::vector<SensorDefinition>{
                                         SensorDefinition{"fan0", true},
                  },
    },
    FanDefinition{"/system/chassis/motherboard/fan1",
                  30,
                  15,
                  15,
                  1,
                  std::vector<SensorDefinition>{
                                         SensorDefinition{"fan1", true},
                  },
    },
    FanDefinition{"/system/chassis/motherboard/fan2",
                  30,
                  15,
                  15,
                  1,
                  std::vector<SensorDefinition>{
                                         SensorDefinition{"fan2", true},
                  },
    },
    FanDefinition{"/system/chassis/motherboard/fan3",
                  30,
                  15,
                  15,
                  1,
                  std::vector<SensorDefinition>{
                                         SensorDefinition{"fan3", true},
                  },
    },
};
