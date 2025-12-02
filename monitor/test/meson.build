phosphor_fan_monitor_include_directories = include_directories('.', '..')
phosphor_fan_monitor_test_include_directories = include_directories('../..')

test_deps = [
    gmock_dep,
    gtest_dep,
    nlohmann_json_dep,
    phosphor_logging_dep,
    sdeventplus_dep,
]

test_sources = [files('../logging.cpp')]

test(
    'power_off_cause',
    executable(
        'power_off_cause',
        'power_off_cause_test.cpp',
        dependencies: test_deps,
        implicit_include_directories: false,
        include_directories: [phosphor_fan_monitor_test_include_directories],
    ),
)

test(
    'power_off_rule_test',
    executable(
        'power_off_rule_test',
        sources: test_sources,
        dependencies: test_deps,
        implicit_include_directories: false,
        include_directories: phosphor_fan_monitor_test_include_directories,
    ),
)

test(
    'multi_chassis_json_parser',
    executable(
        'multi_chassis_json_parser',
        [
            'multi_chassis_json_parser_test.cpp',
            '../multichassis_json_parser.cpp',
            '../conditions.cpp',
        ],
        dependencies: test_deps,
        implicit_include_directories: false,
        include_directories: [phosphor_fan_monitor_test_include_directories],
    ),
)

test(
    'zone_test',
    executable(
        'zone_test',
        [
            'zone_test.cpp',
            '../zone.cpp',
            '../multichassis_json_parser.cpp',
            '../conditions.cpp',
            '../fan.cpp',
            '../tach_sensor.cpp',
            '../json_parser.cpp',
            '../fan_error.cpp',
            '../power_interface.cpp',
            '../../hwmon_ffdc.cpp',
        ],
        sources: test_sources,
        dependencies: test_deps,
        implicit_include_directories: false,
        include_directories: [
            phosphor_fan_monitor_test_include_directories,
            phosphor_fan_monitor_include_directories,
        ],
    ),
)

test(
    'chassis_test',
    executable(
        'chassis_test',
        [
            'chassis_test.cpp',
            '../chassis.cpp',
            '../zone.cpp',
            '../multichassis_json_parser.cpp',
            '../conditions.cpp',
            '../fan.cpp',
            '../tach_sensor.cpp',
            '../json_parser.cpp',
            '../fan_error.cpp',
            '../power_interface.cpp',
            '../../hwmon_ffdc.cpp',
        ],
        sources: test_sources,
        dependencies: test_deps,
        implicit_include_directories: false,
        include_directories: [
            phosphor_fan_monitor_test_include_directories,
            phosphor_fan_monitor_include_directories,
        ],
    ),
)
