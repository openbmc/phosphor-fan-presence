
test_include_directories = include_directories(
    '../..'
)

test_deps=[
    gmock_dep,
    gtest_dep,
]

test(
    'logger_test',
    executable(
        'logger_test',
        'fallbacktest.cpp',
        dependencies:test_deps,
        implicit_include_directories: false,
        include_directories: [test_include_directories]
    )
)
