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

cli11_dep = dependency('cli11', required: false)

if not meson.get_compiler('cpp').has_header_symbol(
    'CLI/CLI.hpp',
    'CLI::App',
    dependencies: cli11_dep,
    required: false)
    cli11_proj = subproject('cli11', required:false)
    assert(cli11_proj.found(), 'CLI11 is required')
    cli11_dep = cli11_proj.get_variable('CLI11_dep')
endif

fmt_dep = dependency('fmt')
json_dep = declare_dependency()

phosphor_dbus_interfaces_dep = dependency('phosphor-dbus-interfaces')
phosphor_logging_dep = dependency('phosphor-logging')
sdbusplus_dep = dependency('sdbusplus')
sdeventplus_dep = dependency('sdeventplus')
stdplus_dep = dependency('stdplus')
systemd_dep = dependency('systemd')

gmock_dep = dependency('gmock', disabler: true)
gtest_dep = dependency('gtest', main: true, disabler: true, required: false)

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

subdir('test')

servicedir = systemd_dep.get_variable('systemdsystemunitdir')
usr_share_dir = '/usr/share/phosphor-fan-presence'

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
if get_option('json-config').enabled()
    conf.set('PRESENCE_USE_JSON', '')
    conf.set('CONTROL_USE_JSON', '')
    conf.set('MONITOR_USE_JSON', '')

    if not cpp.has_header('nlohmann/json.hpp')
        subproject('nlohmann_json')
        json_dep = declare_dependency(
            include_directories: [
                'subprojects/nlohmann-json/single_include',
                'subprojects/nlohmann-json/single_include/nlohmann',
            ]
        )
    endif
    conf_type = 'json'
else
    conf_type = 'yaml'
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

# Service: [name,svcfile]
services = []

build_types = ['ibm,everest','ibm,rainier-1s4u','ibm,rainier-2u','ibm,rainier-4u']

if get_option('control-service').enabled()
    subdir('control')
    services += [['control', 'phosphor-fan-control@.service']]
endif

if get_option('monitor-service').enabled()
    subdir('monitor')
    services += [['monitor', 'phosphor-fan-monitor@.service']]
endif

if get_option('cooling-type-service').enabled()
    libevdev_dep = dependency('libevdev')
    subdir('cooling-type')
endif

if get_option('presence-service').enabled()
    libevdev_dep = dependency('libevdev')
    subdir('presence')
    services += [['presence', 'phosphor-fan-presence-tach@.service']]
endif

if get_option('sensor-monitor-service').enabled()
    subdir('sensor-monitor')
    install_data('sensor-monitor/service_files/sensor-monitor.service',
        install_dir: servicedir)
endif

foreach service : services
    message(service)
    install_data(service[0] + '/service_files/' + conf_type + '/' + service[1],
       install_dir: servicedir)
# everything except sensor monitor needs json config files
    if conf_type=='json'
        foreach build_type : build_types
            install_subdir(service[0] + '/config_files/p10bmc/' + build_type,
               install_dir: usr_share_dir + '/' + service[0])
        endforeach
    endif
endforeach

