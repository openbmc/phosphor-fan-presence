
test_include_directories = include_directories(
    '..'
)

test_deps=[
    fmt_dep,
    gmock_dep,
    gtest_dep,
    json_dep,
    phosphor_dbus_interfaces_dep,
    phosphor_logging_dep,
    sdbusplus_dep
]

test(
    'logger_test',
    executable(
        'logger_test',
        'logger_test.cpp',
        dependencies:test_deps,
        implicit_include_directories: false,
        include_directories: [test_include_directories]
    )
)
