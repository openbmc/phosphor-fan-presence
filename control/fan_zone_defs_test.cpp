/* This is a generated file. */
#include "manager.hpp"

using namespace phosphor::fan::control;

const std::vector<ZoneGroup> Manager::_zoneLayouts
{
    ZoneGroup{std::vector<Condition>{},
              std::vector<ZoneDefinition>{
                  ZoneDefinition{0,
                                 10500,
                                 std::vector<FanDefinition>{
                                     FanDefinition{"/system/chassis/motherboard/fan3",
                                         std::vector<std::string>{
                                            "fan3",
                                         }
                                     },
                                     FanDefinition{"/system/chassis/motherboard/fan2",
                                         std::vector<std::string>{
                                            "fan2",
                                         }
                                     },
                                     FanDefinition{"/system/chassis/motherboard/fan1",
                                         std::vector<std::string>{
                                            "fan1",
                                         }
                                     },
                                     FanDefinition{"/system/chassis/motherboard/fan0",
                                         std::vector<std::string>{
                                            "fan0",
                                         }
                                     },
                                 }
                  },
              }
    },
    ZoneGroup{std::vector<Condition>{},
              std::vector<ZoneDefinition>{
                  ZoneDefinition{0,
                                 10500,
                                 std::vector<FanDefinition>{
                                     FanDefinition{"/system/chassis/motherboard/fan3",
                                         std::vector<std::string>{
                                            "fan3",
                                         }
                                     },
                                     FanDefinition{"/system/chassis/motherboard/fan2",
                                         std::vector<std::string>{
                                            "fan2",
                                         }
                                     },
                                     FanDefinition{"/system/chassis/motherboard/fan0",
                                         std::vector<std::string>{
                                            "fan0",
                                         }
                                     },
                                 }
                  },
              }
    },
};
