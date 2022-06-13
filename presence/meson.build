
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
  libevdev,
  phosphor_logging_dep,
  sdbusplus_dep,
  sdeventplus_dep
]

generated_hpp_dep = custom_target(
    'generated.hpp',
    command: [
        prog_python,
        meson.project_source_root() + '/presence/pfpgen.py',
        'generate-cpp',
        '-i',
        meson.project_source_root() + '/' + conf.get_unquoted('PRESENCE_YAML_FILE')
    ],
    capture: true,
    output: 'generated.hpp'
)

# Only needed for YAML config
if 'yaml' == conf_type
  sources += generated_hpp_dep
endif

phosphor_fan_presence = executable(
  'phosphor-fan-presence-tach',
  sources,
  dependencies : deps,
  implicit_include_directories: false,
  include_directories: phosphor_fan_presence_include_directories,
  install: true,
  install_dir: get_option('bindir')
)


