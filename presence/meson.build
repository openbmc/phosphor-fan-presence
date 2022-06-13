
phosphor_fan_presence_include_directories = include_directories(
    '.',
    '..'
)

sources=[
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
    'tach_detect.cpp'
]

deps=[
    libevdev_dep,
    phosphor_dbus_interfaces_dep,
    phosphor_logging_dep,
    sdbusplus_dep,
    sdeventplus_dep
]

# Only needed for YAML config
if get_option('json-config').disabled()
    generated_hpp_dep = custom_target(
        'generated.hpp',
        input: files(
            './pfpgen.py',
            conf.get_unquoted('PRESENCE_YAML_FILE')
        ),
        command: [
            python_prog, '@INPUT0@',
            'generate-cpp',
            '-i', '@INPUT1@'
        ],
        capture: true,
        output: 'generated.hpp'
    )

    sources += generated_hpp_dep
endif

phosphor_fan_presence = executable(
    'phosphor-fan-presence-tach',
    sources,
    dependencies : deps,
    implicit_include_directories: false,
    include_directories: phosphor_fan_presence_include_directories,
    install: true
)

