
include_dirs=[
    '.',
    '..'
]

deps=[
    phosphor_dbus_interfaces_dep,
    phosphor_logging_dep,
    sdbusplus_dep,
    sdeventplus_dep
]

sources = ['main.cpp']

if conf.has('CONTROL_USE_JSON')
    deps += json_dep
    include_dirs += [
        './json',
        './json/actions',
        './json/triggers'
    ]
    sources += [
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
        'json/actions/target_from_group_max.cpp',
        'json/actions/timer_based_actions.cpp',
        'json/utils/flight_recorder.cpp',
        'json/utils/modifier.cpp',
        'json/utils/pcie_card_metadata.cpp',
        'json/triggers/init.cpp',
        'json/triggers/parameter.cpp',
        'json/triggers/signal.cpp',
        'json/triggers/timer.cpp'
    ]
else
    fan_zone_defs_cpp_dep = custom_target(
        'fan_zone_defs.cpp',
        input: files(
            './gen-fan-zone-defs.py',
            conf.get_unquoted('FAN_DEF_YAML_FILE'),
            conf.get_unquoted('FAN_ZONE_YAML_FILE'),
            conf.get_unquoted('ZONE_EVENTS_YAML_FILE'),
            conf.get_unquoted('ZONE_CONDITIONS_YAML_FILE')
        ),
        command: [
            python_prog, '@INPUT0@',
            '-f','@INPUT1@',
            '-z','@INPUT2@',
            '-e','@INPUT3@',
            '-c','@INPUT4@',
            '-o', 'control'
        ],
        output: 'fan_zone_defs.cpp'
    )

    sources += [
        'actions.cpp',
        'argument.cpp',
        'fan.cpp',
        fan_zone_defs_cpp_dep,
        'manager.cpp',
        'preconditions.cpp',
        'triggers.cpp',
        'utility.cpp',
        'zone.cpp'
    ]

endif

phosphor_fan_control_include_directories = include_directories(include_dirs)
phosphor_fan_control = executable(
    'phosphor-fan-control',
    sources,
    dependencies:deps,
    implicit_include_directories: false,
    include_directories: phosphor_fan_control_include_directories,
    install: true
)

fanctl = executable(
  'fanctl',
  'fanctl.cpp',
      dependencies: [
          cli11_dep,
          fmt_dep,
          sdbusplus_dep
      ],
  include_directories: phosphor_fan_control_include_directories,
  install: true
)
