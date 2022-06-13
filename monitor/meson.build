phosphor_fan_monitor_include_directories = include_directories(
    '.',
    '..'
)

sources = [
  'argument.cpp',
  'conditions.cpp',
  'fan.cpp',
  'fan_error.cpp',
  'json_parser.cpp',
  'logging.cpp',
  'main.cpp',
  'power_interface.cpp',
  'system.cpp',
  'tach_sensor.cpp',
  '../hwmon_ffdc.cpp'
]

fan_monitor_defs_cpp_dep = custom_target(
    'fan_monitor_defs.cpp',
    command: [
        prog_python,
        meson.project_source_root() + '/monitor/gen-fan-monitor-defs.py',
        '-m',
        meson.project_source_root() + '/' + conf.get_unquoted('FAN_MONITOR_YAML_FILE')
    ],
    capture: true,
    output: 'fan_monitor_defs.cpp'
)

# Only needed for YAML config
if 'yaml' == type
  sources += fan_monitor_defs_cpp_dep
endif


phosphor_fan_monitor = executable(
    'phosphor-fan-monitor',
    sources,
    dependencies: [
      phosphor_logging,
      sdeventplus,
      sdbusplus
    ],
    implicit_include_directories: false,
    include_directories: phosphor_fan_monitor_include_directories,
    install: true
)

