phosphor_fan_sensor_monitor_include_directories = include_directories(
    '.',
    '..',
    'domain_event_subscribers'
)

source=[
    'main.cpp',
    'alarm_handlers.cpp',
    'alarm_checker.cpp',
    'dbus_alarm_monitor.cpp',
    'domain_event_subscribers/recovery_subscriber.cpp',
    'shutdown_alarm_monitor.cpp',
    'threshold_alarm_logger.cpp'
]

nlohmann_json = dependency('nlohmann_json')

deps=[
    phosphor_dbus_interfaces_dep,
    phosphor_logging_dep,
    sdeventplus_dep,
    nlohmann_json
]

phosphor_fan_sensor_monitor = executable(
    'sensor-monitor',
    source,
    dependencies: deps,
    implicit_include_directories: false,
    include_directories: phosphor_fan_sensor_monitor_include_directories,
    install: true
)
