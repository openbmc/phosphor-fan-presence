phosphor_fan_presence_include_directories = include_directories(
    '.',
    '..'
)

phosphor_fan_presence = executable(
  'phosphor-fan-presence-tach',
  'anyof.cpp',
  'error_reporter.cpp',
  'fallback.cpp',
  'fan.cpp',
  'get_power_state.cpp',
  'gpio.cpp',
  'json_parser.cpp',
  'logging.cpp',
  'psensor.cpp',
  'tach.cpp',
  'tach_detect.cpp',
    dependencies: [
      libevdev,
      phosphor_logging,
      sdeventplus,
      sdbusplus
    ],
    implicit_include_directories: false,
    include_directories: phosphor_fan_presence_include_directories,
    install: true
)


