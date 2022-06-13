phosphor_fan_sensor_monitor_include_directories = include_directories(
    '.',
    '..'
)

source=[
    'argument.cpp',
    'cooling_type.cpp',
    'cooling_type.hpp',
    'main.cpp'
]

deps=[
    libevdev_dep,
    phosphor_dbus_interfaces_dep,
    phosphor_logging_dep,
    sdeventplus_dep
]

phosphor_fan_cooling_type = executable(
    'phosphor-cooling-type',
    source,
    dependencies: deps,
    implicit_include_directories: false,
    include_directories: ['..'],
    install: true
)
