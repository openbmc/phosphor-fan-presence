phosphor_fan_monitor_include_directories = include_directories(
    '.',
    '..'
)

phosphor_fan_monitor = executable(
    'phosphor-fan-monitor',
    'argument.cpp',
    'conditions.cpp',
    'fan.cpp',
    'fan_error.cpp',
    'fan_monitor_defs.cpp',
    'json_parser.cpp',
    'logging.cpp',
    'main.cpp',
    'power_interface.cpp',
    'system.cpp',
    'tach_sensor.cpp',
    '../hwmon_ffdc.cpp',
    dependencies: [
      phosphor_logging,
      sdeventplus,
      sdbusplus
    ],
    implicit_include_directories: false,
    include_directories: phosphor_fan_monitor_include_directories,
    install: true
)


