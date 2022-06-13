phosphor_fan_sensor_monitor_include_directories = include_directories(
    '.',
    '..'
)

source=[
    'main.cpp',
    'shutdown_alarm_monitor.cpp',
    'threshold_alarm_logger.cpp'
]

deps=[
    phosphor_dbus_interfaces_dep,
    phosphor_logging_dep,
    sdeventplus_dep
]

phosphor_fan_sensor_monitor = executable(
    'sensor-monitor',
    source,
    dependencies: deps,
    implicit_include_directories: false,
    include_directories: ['..'],
    install: true
)
