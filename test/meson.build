
test_include_directories = include_directories(
    '..'
)

test_deps=[
    gmock_dep,
    gtest_dep,
    fmt_dep,
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
