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

build_tests = get_option('tests')

if get_option('oe-sdk').enabled()
  # Setup OE SYSROOT
  OECORE_TARGET_SYSROOT = run_command('sh', '-c', 'echo $OECORE_TARGET_SYSROOT').stdout().strip()
  if OECORE_TARGET_SYSROOT == ''
    error('Unable to get $OECORE_TARGET_SYSROOT, check your environment.')
  endif
  message('OE_SYSROOT: ' + OECORE_TARGET_SYSROOT)
  rpath = ':'.join([OECORE_TARGET_SYSROOT + '/lib', OECORE_TARGET_SYSROOT + '/usr/lib'])
  ld_so = run_command('sh', '-c', 'find ' + OECORE_TARGET_SYSROOT + '/lib/ld-*.so | sort -r -n | head -n1').stdout().strip()
  dynamic_linker = ['-Wl,-dynamic-linker,' + ld_so]
else
  dynamic_linker = []
endif


gmock = dependency('gmock', disabler: true, required: build_tests)
gtest = dependency('gtest', main: true, disabler: true, required: build_tests)
libevdev = dependency('libevdev')
phosphor_logging = dependency('phosphor-logging')
sdbusplus = dependency('sdbusplus')
sdeventplus = dependency('sdeventplus')
stdplus = dependency('stdplus')
fmt = dependency('fmt')
systemd = dependency('systemd')

servicedir = systemd.get_pkgconfig_variable('systemdsystemunitdir')

services = [
    ['fan-control', 'phoshor-fan-control@.service'],
    ['fan-monitor', 'phosphor-fan-monitor@.service'],
    ['fan-presence', 'phosphor-fan-presence-tach@.service'],
]

# foreach service : services
#     if get_option(service[0])
#         configure_file(input: '<something>/service_files/' + service[1],
#                  output: service[1],
#                  copy: true,
#                  install_dir: servicedir)
#     endif
# endforeach

# Get the power sequencer class name
sequencer = get_option('power_sequencer')
if sequencer == 'ucd90160'
    sequencer_class = 'UCD90160'
elif sequencer == 'mihawk-cpld'
    sequencer_class = 'MihawkCPLD'
else
    # power sequencer is incorrect
    error('power sequencer is incorrect')
endif

conf = configuration_data()
conf.set10(
    'MONITOR_USE_JSON', get_option('use-json'))
conf.set10(
    'CONTROL_USE_JSON', get_option('use-json'))
conf.set_quoted(
    'CONTROL_BUSNAME', 'xyz.openbmc_project.Control.Thermal')
conf.set_quoted(
    'CONTROL_OBJPATH', '/xyz/openbmc_project/control/thermal')
conf.set_quoted(
    'THERMAL_ALERT_BUSNAME', 'xyz.openbmc_project.Thermal.Alert')
conf.set_quoted(
    'THERMAL_ALERT_OBJPATH', '/xyz/openbmc_project/alerts/thermal_fault_alert')
conf.set_quoted(
    'CONTROL_PERSIST_ROOT_PATH', '/var/lib/phosphor-fan-presence/control')
conf.set(
    'NUM_PRESENCE_LOG_ENTRIES', 50)
conf.set(
    'NUM_MONITOR_LOG_ENTRIES', 75)

configure_file(output: 'config.h', configuration: conf)

subdir('control')
subdir('monitor')
subdir('presence')

if get_option('tests').enabled()
    subdir('test')
endif
