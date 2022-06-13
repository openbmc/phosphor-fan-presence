

phosphor_logging = dependency('phosphor-logging')

phosphor_fan_control_include_directories = include_directories(
    './json',
    './json/actions',
    './json/actions',
    './json/triggers',
    '.',
    '..'
)

phosphor_fan_monitor = executable(
    'phosphor-fan-control',
    'actions.cpp',
    'argument.cpp',
    'fan.cpp',
    'fan_zone_defs.cpp',
    'main.cpp',
    'manager.cpp',
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
    dependencies: [
      phosphor_logging,
      sdeventplus,
      sdbusplus
    ],
    implicit_include_directories: false,
    include_directories: phosphor_fan_control_include_directories,
    install: true
)

fanctl = executable(
  'fanctl',
  'fanctl.cpp',
    dependencies: [
      fmt,
      sdbusplus
    ],
  include_directories: phosphor_fan_control_include_directories,
  install: true
)
