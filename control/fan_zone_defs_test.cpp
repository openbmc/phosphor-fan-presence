/* This is a generated file. */
#include "manager.hpp"

using namespace phosphor::fan::control;

const std::vector<ZoneGroup> Manager::_zoneLayouts
{
    std::make_tuple(
            std::vector<Condition>{},
            std::vector<ZoneDefinition>{
                std::make_tuple(0,
                                10500,
                                std::vector<FanDefinition>{
                                    std::make_tuple("/system/chassis/motherboard/fan3",
                                        std::vector<std::string>{
                                           "fan3",
                                        }
                                    ),
                                    std::make_tuple("/system/chassis/motherboard/fan2",
                                        std::vector<std::string>{
                                           "fan2",
                                        }
                                    ),
                                    std::make_tuple("/system/chassis/motherboard/fan1",
                                        std::vector<std::string>{
                                           "fan1",
                                        }
                                    ),
                                    std::make_tuple("/system/chassis/motherboard/fan0",
                                        std::vector<std::string>{
                                           "fan0",
                                        }
                                    ),
                                }
                ),
            }
    ),
    std::make_tuple(
            std::vector<Condition>{},
            std::vector<ZoneDefinition>{
                std::make_tuple(0,
                                10500,
                                std::vector<FanDefinition>{
                                    std::make_tuple("/system/chassis/motherboard/fan3",
                                        std::vector<std::string>{
                                           "fan3",
                                        }
                                    ),
                                    std::make_tuple("/system/chassis/motherboard/fan2",
                                        std::vector<std::string>{
                                           "fan2",
                                        }
                                    ),
                                    std::make_tuple("/system/chassis/motherboard/fan0",
                                        std::vector<std::string>{
                                           "fan0",
                                        }
                                    ),
                                }
                ),
            }
    ),
};
