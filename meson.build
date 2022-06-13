project(
    'phosphor-fan-presence',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++20',
        'buildtype=debugoptimized',
        'prefix=/usr'
    ],
    license: 'Apache-2.0',
    version: '1.0',
    meson_version: '>=0.57.0',
)

prog_python = import('python').find_installation('python3')

cpp = meson.get_compiler('cpp')

fmt_dep = dependency('fmt') 
libevdev = dependency('libevdev')
phosphor_logging_dep = dependency('phosphor-logging')
sdbusplus_dep = dependency('sdbusplus')
sdeventplus_dep = dependency('sdeventplus')
stdplus_dep = dependency('stdplus')
systemd_dep = dependency('systemd')

servicedir = systemd_dep.get_pkgconfig_variable('systemdsystemunitdir')

# default config type
conf_type = 'json'

# Service: [name,conf_type,svcfile]
services = []

conf = configuration_data()

# Control
conf.set_quoted(
    'CONTROL_PERSIST_ROOT_PATH', get_option('control-persist-root-path'))
conf.set_quoted(
    'CONTROL_BUSNAME', get_option('control-busname'))
conf.set_quoted(
    'CONTROL_OBJPATH', get_option('control-objpath'))
conf.set_quoted(
    'CONTROL_PERSIST_ROOT_PATH', get_option('control-persist-root-path'))

conf.set_quoted(
    'FAN_DEF_YAML_FILE', get_option('fan-def-yaml-file'))
conf.set_quoted(
    'FAN_ZONE_YAML_FILE', get_option('fan-zone-yaml-file'))
conf.set_quoted(
    'ZONE_EVENTS_YAML_FILE', get_option('zone-events-yaml-file'))
conf.set_quoted(
    'ZONE_CONDITIONS_YAML_FILE', get_option('zone-conditions-yaml-file'))

# Monitor
conf.set(
    'NUM_MONITOR_LOG_ENTRIES', get_option('num-monitor-log-entries'))
conf.set_quoted(
    'THERMAL_ALERT_BUSNAME', get_option('thermal-alert-busname'))
conf.set_quoted(
    'THERMAL_ALERT_OBJPATH', get_option('thermal-alert-objpath'))
conf.set_quoted(
    'FAN_MONITOR_YAML_FILE', get_option('fan-monitor-yaml-file'))

# JSON-or-YAML (all programs)
if get_option('enable-json').enabled()
    conf.set10('PRESENCE_USE_JSON', true)
    conf.set10('CONTROL_USE_JSON', true)
    conf.set10('MONITOR_USE_JSON', true)
endif
conf.set(
    'NUM_PRESENCE_LOG_ENTRIES', get_option('num-presence-log-entries'))
conf.set_quoted(
    'PRESENCE_YAML_FILE', get_option('presence-config'))

# Sensor
if get_option('enable-host-state').enabled()
    conf.set10('ENABLE_HOST_STATE', true)
endif

conf.set_quoted(
    'SENSOR_MONITOR_PERSIST_ROOT_PATH', get_option('sensor-monitor-root-path'))
conf.set(
    'SHUTDOWN_ALARM_HARD_SHUTDOWN_DELAY_MS', get_option('sensor-monitor-hard-shutdown-delay'))
conf.set(
    'SHUTDOWN_ALARM_SOFT_SHUTDOWN_DELAY_MS', get_option('sensor-monitor-soft-shutdown-delay'))

configure_file(output: 'config.h', configuration: conf)

if get_option('enable-json').disabled()
    conf_type = 'yaml'
endif

if get_option('disable-control').disabled()
    subdir('control')
    services += [['control', conf_type, 'phosphor-fan-control@.service']]
endif

if get_option('disable-monitor').disabled()
    subdir('monitor')
    services += [['monitor', conf_type, 'phosphor-fan-monitor@.service']]
endif

if get_option('disable-presence').disabled()
    subdir('presence')
    services += [['presence', conf_type, 'phosphor-fan-presence-tach@.service']]
endif

if get_option('enable-sensor-monitor').enabled()
    subdir('sensor-monitor')
    services += [['sensor-monitor', '', 'sensor-monitor.service']]
endif

foreach service : services
message(service)
    configure_file(input: service[0] + '/service_files/' + service[1] + '/' + service[2],
       output: service[2],
       copy: true,
       install_dir: servicedir)
endforeach

build_tests = get_option('tests')
if build_tests.enabled()
  subdir('test')
endif

