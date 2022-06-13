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

python_prog = find_program('python3', native: true)

cpp = meson.get_compiler('cpp')

fmt_dep = dependency('fmt')
libevdev_dep = dependency('libevdev')
phosphor_logging_dep = dependency('phosphor-logging')
sdbusplus_dep = dependency('sdbusplus')
sdeventplus_dep = dependency('sdeventplus')
stdplus_dep = dependency('stdplus')
systemd_dep = dependency('systemd')

build_tests = get_option('tests')
gmock_dep = dependency('gmock', disabler: true, required: build_tests)
gtest_dep = dependency('gtest', main: true, disabler: true, required: build_tests)

if not gtest_dep.found() or not gmock_dep.found()
    gtest_proj = import('cmake').subproject('googletest', required: false)
    if gtest_proj.found()
        gtest_dep = declare_dependency(
            dependencies: [
                dependency('threads'),
                gtest_proj.dependency('gtest'),
                gtest_proj.dependency('gtest_main'),
            ]
        )
        gmock_dep = gtest_proj.dependency('gmock')
    else
        assert(
            not get_option('tests').enabled(),
            'Googletest is required if tests are enabled'
        )
    endif
endif

servicedir = systemd_dep.get_variable('systemdsystemunitdir')

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
    conf.set('PRESENCE_USE_JSON', '')
    conf.set('CONTROL_USE_JSON', '')
    conf.set('MONITOR_USE_JSON', '')
endif

conf.set(
    'NUM_PRESENCE_LOG_ENTRIES', get_option('num-presence-log-entries'))
conf.set_quoted(
    'PRESENCE_YAML_FILE', get_option('presence-config'))

# Sensor
if get_option('enable-host-state').enabled()
    conf.set('ENABLE_HOST_STATE', '')
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
else
  conf_type = 'json'
endif

# Service: [name,conf_type,svcfile]
services = []

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
# has no conf type
    services += [['sensor-monitor', '', 'sensor-monitor.service']]
endif

foreach service : services
message(service)
    install_data(service[0] + '/service_files/' + service[1] + '/' + service[2],
       install_dir: servicedir)
endforeach

build_tests = get_option('tests')
if build_tests.enabled()
  subdir('test')
endif
