phosphor_fan_sensor_monitor_include_directories = include_directories(
    '.',
    '..'
)

phosphor_fan_sensor_monitor = executable(
    'phosphor-fan-monitor',
    'main.cpp',
    'shutdown_alarm_monitor.cpp',
    'threshold_alarm_logger.cpp',
    dependencies: [
      phosphor_logging,
      sdeventplus
    ],
    implicit_include_directories: false,
    include_directories: phosphor_fan_sensor_monitor_include_directories,
    install: true
)
