
test_include_directories = include_directories('../..')

test_deps = [
    gmock_dep,
    gtest_dep,
    sdbusplus_dep,
    sdeventplus_dep,
    phosphor_logging_dep,
]

test(
    'fallback_test',
    executable(
        'fallback_test',
        'fallbacktest.cpp',
        '../fallback.cpp',
        '../psensor.cpp',
        dependencies: test_deps,
        implicit_include_directories: false,
        include_directories: [test_include_directories],
    ),
)
