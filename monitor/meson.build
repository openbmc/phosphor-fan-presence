phosphor_fan_monitor_include_directories = include_directories(
    '.',
    '..'
)

sources=[
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

deps=[
    json_dep,
    phosphor_dbus_interfaces_dep,
    phosphor_logging_dep,
    sdbusplus_dep,
    sdeventplus_dep
]

fan_monitor_defs_cpp_dep = custom_target(
    'fan_monitor_defs.cpp',
    input: files(
        './gen-fan-monitor-defs.py',
        conf.get_unquoted('FAN_MONITOR_YAML_FILE'),
    ),
    command: [
        python_prog, '@INPUT0@',
        '-m','@INPUT1@',
        '-o','monitor'
    ],
    output: 'fan_monitor_defs.cpp'
)

# Only needed for YAML config
if get_option('enable-json').disabled()
    sources += fan_monitor_defs_cpp_dep
endif

phosphor_fan_monitor = executable(
    'phosphor-fan-monitor',
    sources,
    dependencies: deps,
    implicit_include_directories: false,
    include_directories: phosphor_fan_monitor_include_directories,
    install: true
)

if build_tests.enabled()
    subdir('test')
endif

