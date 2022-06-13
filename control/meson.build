phosphor_fan_control_include_directories = include_directories(
    '.',
    '..',
    './json',
    './json/actions',
    './json/triggers'
)

sources = [
    'actions.cpp',
    'argument.cpp',
    'fan.cpp',
    'main.cpp',
    'preconditions.cpp',
    'triggers.cpp',
    'utility.cpp',
    'zone.cpp',
    'json/dbus_zone.cpp',
    'json/event.cpp',
    'json/fan.cpp',
    'json/group.cpp',
    'json/manager.cpp',
    'json/profile.cpp',
    'json/zone.cpp',
    'json/actions/count_state_floor.cpp',
    'json/actions/count_state_target.cpp',
    'json/actions/default_floor.cpp',
    'json/actions/get_managed_objects.cpp',
    'json/actions/mapped_floor.cpp',
    'json/actions/missing_owner_target.cpp',
    'json/actions/net_target_decrease.cpp',
    'json/actions/net_target_increase.cpp',
    'json/actions/override_fan_target.cpp',
    'json/actions/pcie_card_floors.cpp',
    'json/actions/request_target_base.cpp',
    'json/actions/set_parameter_from_group_max.cpp',
    'json/actions/timer_based_actions.cpp',
    'json/utils/flight_recorder.cpp',
    'json/utils/modifier.cpp',
    'json/utils/pcie_card_metadata.cpp',
    'json/triggers/init.cpp',
    'json/triggers/parameter.cpp',
    'json/triggers/signal.cpp',
    'json/triggers/timer.cpp',
]

deps=[
  phosphor_logging_dep,
  sdbusplus_dep,
  sdeventplus_dep
]

fan_zone_defs_cpp_dep = custom_target(
    'fan_zone_defs.cpp',
    command: [
        prog_python,
        meson.project_source_root() + '/control/gen-fan-zone-defs.py',
        '-f',
        meson.project_source_root() + '/' + conf.get_unquoted('FAN_DEF_YAML_FILE'),
        '-z',
        meson.project_source_root() + '/' + conf.get_unquoted('FAN_ZONE_YAML_FILE'),
        '-e',
        meson.project_source_root() + '/' + conf.get_unquoted('ZONE_EVENTS_YAML_FILE'),
        '-c',
        meson.project_source_root() + '/' + conf.get_unquoted('ZONE_CONDITIONS_YAML_FILE'),
        '-o',
        'control'
    ],
    output: 'fan_zone_defs.cpp'
)

# Only needed for YAML config
if 'yaml' == conf_type
  sources += fan_zone_defs_cpp_dep
  sources += 'manager.cpp'
endif

phosphor_fan_monitor = executable(
    'phosphor-fan-control',
    sources,
    dependencies:deps,
    implicit_include_directories: false,
    include_directories: phosphor_fan_control_include_directories,
    install: true,
    install_dir: get_option('bindir')
)

fanctl = executable(
  'fanctl',
  'fanctl.cpp',
    dependencies: [
      fmt_dep,
      sdbusplus_dep
    ],
  include_directories: phosphor_fan_control_include_directories,
  install: true
)
