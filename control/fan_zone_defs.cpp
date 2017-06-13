/* This is a generated file. */
#include <sdbusplus/bus.hpp>
#include "manager.hpp"
#include "functor.hpp"
#include "actions.hpp"
#include "handlers.hpp"

using namespace phosphor::fan::control;
using namespace sdbusplus::bus::match::rules;

const unsigned int Manager::_powerOnDelay{20};

static std::map<int64_t, uint64_t> test = {{9000, 6000}, {8000, 4000}, {10000, 8000}};
//static_cast<std::map<int64_t, uint64_t>>(std::map<int64_t, uint64_t>{{8000, 4000}, {9000, 6000}, {10000, 8000}})

const std::vector<ZoneGroup> Manager::_zoneLayouts
{
    ZoneGroup{
        std::vector<Condition>{},
        std::vector<ZoneDefinition>{
            ZoneDefinition{
                0,
                10500,
                6000,
                std::vector<FanDefinition>{
                    FanDefinition{
                        "/system/chassis/motherboard/fan0",
                        std::vector<std::string>{
                            "fan0",
                        }
                    },
                    FanDefinition{
                        "/system/chassis/motherboard/fan1",
                        std::vector<std::string>{
                            "fan1",
                        }
                    },
                    FanDefinition{
                        "/system/chassis/motherboard/fan2",
                        std::vector<std::string>{
                            "fan2",
                        }
                    },
                    FanDefinition{
                        "/system/chassis/motherboard/fan3",
                        std::vector<std::string>{
                            "fan3",
                        }
                    },
                },
                std::vector<SetSpeedEvent>{
                    SetSpeedEvent{
                        Group{
                        {
                            "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan0",
                            {"xyz.openbmc_project.Inventory.Item",
                             "Present"}
                        },
                        {
                            "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan1",
                            {"xyz.openbmc_project.Inventory.Item",
                             "Present"}
                        },
                        {
                            "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan2",
                            {"xyz.openbmc_project.Inventory.Item",
                             "Present"}
                        },
                        {
                            "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan3",
                            {"xyz.openbmc_project.Inventory.Item",
                             "Present"}
                        },
                        },
                        make_action(action::count_state_before_speed(
                            static_cast<size_t>(1),
                            static_cast<bool>(false),
                            static_cast<uint64_t>(10500)
                        )),
                        std::vector<PropertyChange>{
                            PropertyChange{
                                interface("org.freedesktop.DBus.Properties") +
                                member("PropertiesChanged") +
                                type::signal() +
                                path("/xyz/openbmc_project/inventory/system/chassis/motherboard/fan0") +
                                arg0namespace("xyz.openbmc_project.Inventory.Item"),
                                make_handler(propertySignal<bool>(
                                    "xyz.openbmc_project.Inventory.Item",
                                    "Present",
                                    handler::setProperty<bool>(
                                        "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan0",
                                        "xyz.openbmc_project.Inventory.Item",
                                        "Present"
                                    )
                                ))
                            },
                            PropertyChange{
                                interface("org.freedesktop.DBus.Properties") +
                                member("PropertiesChanged") +
                                type::signal() +
                                path("/xyz/openbmc_project/inventory/system/chassis/motherboard/fan1") +
                                arg0namespace("xyz.openbmc_project.Inventory.Item"),
                                make_handler(propertySignal<bool>(
                                    "xyz.openbmc_project.Inventory.Item",
                                    "Present",
                                    handler::setProperty<bool>(
                                        "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan1",
                                        "xyz.openbmc_project.Inventory.Item",
                                        "Present"
                                    )
                                ))
                            },
                            PropertyChange{
                                interface("org.freedesktop.DBus.Properties") +
                                member("PropertiesChanged") +
                                type::signal() +
                                path("/xyz/openbmc_project/inventory/system/chassis/motherboard/fan2") +
                                arg0namespace("xyz.openbmc_project.Inventory.Item"),
                                make_handler(propertySignal<bool>(
                                    "xyz.openbmc_project.Inventory.Item",
                                    "Present",
                                    handler::setProperty<bool>(
                                        "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan2",
                                        "xyz.openbmc_project.Inventory.Item",
                                        "Present"
                                    )
                                ))
                            },
                            PropertyChange{
                                interface("org.freedesktop.DBus.Properties") +
                                member("PropertiesChanged") +
                                type::signal() +
                                path("/xyz/openbmc_project/inventory/system/chassis/motherboard/fan3") +
                                arg0namespace("xyz.openbmc_project.Inventory.Item"),
                                make_handler(propertySignal<bool>(
                                    "xyz.openbmc_project.Inventory.Item",
                                    "Present",
                                    handler::setProperty<bool>(
                                        "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan3",
                                        "xyz.openbmc_project.Inventory.Item",
                                        "Present"
                                    )
                                ))
                            },
                        }
                    },
                    SetSpeedEvent{
                        Group{
                        {
                            "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan0",
                            {"xyz.openbmc_project.State.Decorator.OperationalStatus",
                             "Functional"}
                        },
                        {
                            "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan1",
                            {"xyz.openbmc_project.State.Decorator.OperationalStatus",
                             "Functional"}
                        },
                        {
                            "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan2",
                            {"xyz.openbmc_project.State.Decorator.OperationalStatus",
                             "Functional"}
                        },
                        {
                            "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan3",
                            {"xyz.openbmc_project.State.Decorator.OperationalStatus",
                             "Functional"}
                        },
                        },
                        make_action(action::count_state_before_speed(
                            static_cast<size_t>(1),
                            static_cast<bool>(false),
                            static_cast<uint64_t>(10500)
                        )),
                        std::vector<PropertyChange>{
                            PropertyChange{
                                interface("org.freedesktop.DBus.Properties") +
                                member("PropertiesChanged") +
                                type::signal() +
                                path("/xyz/openbmc_project/inventory/system/chassis/motherboard/fan0") +
                                arg0namespace("xyz.openbmc_project.State.Decorator.OperationalStatus"),
                                make_handler(propertySignal<bool>(
                                    "xyz.openbmc_project.State.Decorator.OperationalStatus",
                                    "Functional",
                                    handler::setProperty<bool>(
                                        "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan0",
                                        "xyz.openbmc_project.State.Decorator.OperationalStatus",
                                        "Functional"
                                    )
                                ))
                            },
                            PropertyChange{
                                interface("org.freedesktop.DBus.Properties") +
                                member("PropertiesChanged") +
                                type::signal() +
                                path("/xyz/openbmc_project/inventory/system/chassis/motherboard/fan1") +
                                arg0namespace("xyz.openbmc_project.State.Decorator.OperationalStatus"),
                                make_handler(propertySignal<bool>(
                                    "xyz.openbmc_project.State.Decorator.OperationalStatus",
                                    "Functional",
                                    handler::setProperty<bool>(
                                        "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan1",
                                        "xyz.openbmc_project.State.Decorator.OperationalStatus",
                                        "Functional"
                                    )
                                ))
                            },
                            PropertyChange{
                                interface("org.freedesktop.DBus.Properties") +
                                member("PropertiesChanged") +
                                type::signal() +
                                path("/xyz/openbmc_project/inventory/system/chassis/motherboard/fan2") +
                                arg0namespace("xyz.openbmc_project.State.Decorator.OperationalStatus"),
                                make_handler(propertySignal<bool>(
                                    "xyz.openbmc_project.State.Decorator.OperationalStatus",
                                    "Functional",
                                    handler::setProperty<bool>(
                                        "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan2",
                                        "xyz.openbmc_project.State.Decorator.OperationalStatus",
                                        "Functional"
                                    )
                                ))
                            },
                            PropertyChange{
                                interface("org.freedesktop.DBus.Properties") +
                                member("PropertiesChanged") +
                                type::signal() +
                                path("/xyz/openbmc_project/inventory/system/chassis/motherboard/fan3") +
                                arg0namespace("xyz.openbmc_project.State.Decorator.OperationalStatus"),
                                make_handler(propertySignal<bool>(
                                    "xyz.openbmc_project.State.Decorator.OperationalStatus",
                                    "Functional",
                                    handler::setProperty<bool>(
                                        "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan3",
                                        "xyz.openbmc_project.State.Decorator.OperationalStatus",
                                        "Functional"
                                    )
                                ))
                            },
                        }
                    },
                    SetSpeedEvent{
                        Group{
                        {
                            "/xyz/openbmc_project/sensors/fan_tach/fan0",
                            {"xyz.openbmc_project.Sensor.Value",
                             "Value"}
                        },
                        {
                            "/xyz/openbmc_project/sensors/fan_tach/fan1",
                            {"xyz.openbmc_project.Sensor.Value",
                             "Value"}
                        },
                        {
                            "/xyz/openbmc_project/sensors/fan_tach/fan2",
                            {"xyz.openbmc_project.Sensor.Value",
                             "Value"}
                        },
                        {
                            "/xyz/openbmc_project/sensors/fan_tach/fan3",
                            {"xyz.openbmc_project.Sensor.Value",
                             "Value"}
                        },
                        },
                        make_action(action::set_floor_from_average_sensor_value(
                            static_cast<std::map<int64_t, uint64_t>>(std::map<int64_t, uint64_t>{{8000, 4000}, {9000, 6000}, {10000, 8000}})
                        )),
                        std::vector<PropertyChange>{
                            PropertyChange{
                                interface("org.freedesktop.DBus.Properties") +
                                member("PropertiesChanged") +
                                type::signal() +
                                path("/xyz/openbmc_project/sensors/fan_tach/fan0") +
                                arg0namespace("xyz.openbmc_project.Sensor.Value"),
                                make_handler(propertySignal<int64_t>(
                                    "xyz.openbmc_project.Sensor.Value",
                                    "Value",
                                    handler::setProperty<int64_t>(
                                        "/xyz/openbmc_project/sensors/fan_tach/fan0",
                                        "xyz.openbmc_project.Sensor.Value",
                                        "Value"
                                    )
                                ))
                            },
                            PropertyChange{
                                interface("org.freedesktop.DBus.Properties") +
                                member("PropertiesChanged") +
                                type::signal() +
                                path("/xyz/openbmc_project/sensors/fan_tach/fan1") +
                                arg0namespace("xyz.openbmc_project.Sensor.Value"),
                                make_handler(propertySignal<int64_t>(
                                    "xyz.openbmc_project.Sensor.Value",
                                    "Value",
                                    handler::setProperty<int64_t>(
                                        "/xyz/openbmc_project/sensors/fan_tach/fan1",
                                        "xyz.openbmc_project.Sensor.Value",
                                        "Value"
                                    )
                                ))
                            },
                            PropertyChange{
                                interface("org.freedesktop.DBus.Properties") +
                                member("PropertiesChanged") +
                                type::signal() +
                                path("/xyz/openbmc_project/sensors/fan_tach/fan2") +
                                arg0namespace("xyz.openbmc_project.Sensor.Value"),
                                make_handler(propertySignal<int64_t>(
                                    "xyz.openbmc_project.Sensor.Value",
                                    "Value",
                                    handler::setProperty<int64_t>(
                                        "/xyz/openbmc_project/sensors/fan_tach/fan2",
                                        "xyz.openbmc_project.Sensor.Value",
                                        "Value"
                                    )
                                ))
                            },
                            PropertyChange{
                                interface("org.freedesktop.DBus.Properties") +
                                member("PropertiesChanged") +
                                type::signal() +
                                path("/xyz/openbmc_project/sensors/fan_tach/fan3") +
                                arg0namespace("xyz.openbmc_project.Sensor.Value"),
                                make_handler(propertySignal<int64_t>(
                                    "xyz.openbmc_project.Sensor.Value",
                                    "Value",
                                    handler::setProperty<int64_t>(
                                        "/xyz/openbmc_project/sensors/fan_tach/fan3",
                                        "xyz.openbmc_project.Sensor.Value",
                                        "Value"
                                    )
                                ))
                            },
                        }
                    },
                }
            },
        }
    },
};
