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
    meson_version: '>=0.58.0',
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

if(get_option('tests').enabled())
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
endif


servicedir = systemd_dep.get_variable('systemdsystemunitdir')
usr_share_dir = '/usr/share/phosphor-fan-presence'

conf = configuration_data()

# Control
conf.set_quoted(
    'CONTROL_PERSIST_ROOT_PATH', get_option('control-persist-root-path'))
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

# Fan control can be in YAML mode when everything else is in JSON mode
control_conf_type = 'yaml'
if get_option('json-config').enabled() and get_option('json-control').enabled()
    control_conf_type = 'json'
endif

# Monitor
conf.set(
    'NUM_MONITOR_LOG_ENTRIES', get_option('num-monitor-log-entries'))
conf.set_quoted(
    'FAN_MONITOR_YAML_FILE', get_option('fan-monitor-yaml-file'))
conf.set('DELAY_HOST_CONTROL', get_option('delay-host-control'))
if get_option('monitor-use-host-state').enabled()
    conf.set('MONITOR_USE_HOST_STATE', '')
endif

# JSON-or-YAML (all programs)
if get_option('json-config').enabled()
    conf.set('PRESENCE_USE_JSON', '')
    if control_conf_type == 'json'
        conf.set('CONTROL_USE_JSON', '')
    endif
    conf.set('MONITOR_USE_JSON', '')

    if not cpp.has_header('nlohmann/json.hpp')
        json_dep = dependency('nlohmann_json')
    endif
    conf_type = 'json'
else
    conf_type = 'yaml'
endif

# Sensor Monitor
conf.set_quoted('SENSOR_MONITOR_PERSIST_ROOT_PATH',
                get_option('sensor-monitor-persist-root-path'))

if get_option('use-host-power-state').enabled()
    conf.set('ENABLE_HOST_STATE', '')
endif

conf.set(
    'SHUTDOWN_ALARM_HARD_SHUTDOWN_DELAY_MS', get_option('sensor-monitor-hard-shutdown-delay'))
conf.set(
    'SHUTDOWN_ALARM_SOFT_SHUTDOWN_DELAY_MS', get_option('sensor-monitor-soft-shutdown-delay'))

# Presence
conf.set(
    'NUM_PRESENCE_LOG_ENTRIES', get_option('num-presence-log-entries'))
conf.set_quoted(
    'PRESENCE_YAML_FILE', get_option('presence-config'))

configure_file(output: 'config.h', configuration: conf)

# Service: [name,[svcfiles]]
services = []

if get_option('control-service').enabled()
    subdir('control')
    service_files = ['phosphor-fan-control@.service']
    if control_conf_type == 'yaml'
        service_files += 'phosphor-fan-control-init@.service'
    endif
    services += [['control', service_files]]
endif

if get_option('monitor-service').enabled()
    subdir('monitor')
    service_files = ['phosphor-fan-monitor@.service']
    if not get_option('json-config').enabled()
        service_files += 'phosphor-fan-monitor-init@.service'
    endif
    services += [['monitor', service_files]]
endif

if get_option('cooling-type-service').enabled()
    libevdev_dep = dependency('libevdev')
    subdir('cooling-type')
endif

if get_option('presence-service').enabled()
    libevdev_dep = dependency('libevdev')
    subdir('presence')
    services += [['presence', ['phosphor-fan-presence-tach@.service']]]
endif

if get_option('sensor-monitor-service').enabled()
    subdir('sensor-monitor')
    install_data('sensor-monitor/service_files/sensor-monitor.service',
        install_dir: servicedir)
endif

foreach service : services
    this_conf_type = conf_type

    if service[0] == 'control'
        this_conf_type = control_conf_type
    endif

    foreach service_file : service[1]
        install_data(service[0] / 'service_files' / this_conf_type / service_file,
                     install_dir: servicedir)
    endforeach

    if this_conf_type == 'json'
        fs = import('fs')
        dir = meson.current_source_dir() / service[0] / 'config_files' / get_option('machine-name')
        if fs.is_dir(dir)
            install_subdir(service[0] / 'config_files' / get_option('machine-name'),
                          install_dir: usr_share_dir / service[0],
                          strip_directory: true)
        endif
    endif
endforeach

