# Initialization
AC_PREREQ([2.69])
AC_INIT([phosphor-fan-presence], [1.0],
[https://github.com/openbmc/phosphor-fan-presence/issues])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Use gcc-ar and gcc-ranlib for static libraries built with -flto
AC_SUBST([AR], [${ac_tool_prefix}gcc-ar])
AC_SUBST([RANLIB], [${ac_tool_prefix}gcc-ranlib])

# Checks for programs.
AC_PROG_CXX
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Python
AM_PATH_PYTHON([3], [AC_SUBST([PYTHON], [echo "$PYTHON"])],
[AC_MSG_ERROR([Minimum python version(3) not found])])

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_17([noext])
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CXXFLAGS])

# Checks for libraries.
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus])
PKG_CHECK_MODULES([SDEVENTPLUS], [sdeventplus])
PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging])
PKG_CHECK_MODULES([SYSTEMD], [libsystemd >= 221])
PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [phosphor-dbus-interfaces])
PKG_CHECK_MODULES([LIBEVDEV], [libevdev])
PKG_CHECK_MODULES([STDPLUS], [stdplus])
PKG_CHECK_MODULES([FMT], [fmt])

# Checks for library functions.
LT_INIT # Required for systemd linking

# Check/set gtest specific functions.
AX_PTHREAD([GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=1"],[GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=0"])
AC_SUBST(GTEST_CPPFLAGS)
AC_ARG_ENABLE([oe-sdk],
    AS_HELP_STRING([--enable-oe-sdk], [Link testcases absolutely against OE SDK so they can be ran within it.])
)
AC_ARG_VAR(OECORE_TARGET_SYSROOT,
    [Path to the OE SDK SYSROOT])
AS_IF([test "x$enable_oe_sdk" == "xyes"],
    AS_IF([test "x$OECORE_TARGET_SYSROOT" == "x"],
          AC_MSG_ERROR([OECORE_TARGET_SYSROOT must be set with --enable-oe-sdk])
    )
    AC_MSG_NOTICE([Enabling OE-SDK at $OECORE_TARGET_SYSROOT])
    [
        testcase_flags="-Wl,-rpath,\${OECORE_TARGET_SYSROOT}/lib"
        testcase_flags="${testcase_flags} -Wl,-rpath,\${OECORE_TARGET_SYSROOT}/usr/lib"
        testcase_flags="${testcase_flags} -Wl,-dynamic-linker,`find \${OECORE_TARGET_SYSROOT}/lib/ld-*.so | sort -r -n | head -n1`"
    ]
    AC_SUBST([OESDK_TESTCASE_FLAGS], [$testcase_flags])
)

PKG_PROG_PKG_CONFIG
AC_ARG_WITH([systemdsystemunitdir],
     [AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files])],,
     [with_systemdsystemunitdir=auto])
AS_IF([test "x$with_systemdsystemunitdir" = "xyes" -o "x$with_systemdsystemunitdir" = "xauto"], [
     def_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)

     AS_IF([test "x$def_systemdsystemunitdir" = "x"],
   [AS_IF([test "x$with_systemdsystemunitdir" = "xyes"],
    [AC_MSG_ERROR([systemd support requested but pkg-config unable to query systemd package])])
    with_systemdsystemunitdir=no],
   [with_systemdsystemunitdir="$def_systemdsystemunitdir"])])
AS_IF([test "x$with_systemdsystemunitdir" != "xno"],
      [AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])])

AC_ARG_ENABLE([json],
    AS_HELP_STRING([--enable-json], [Use json at runtime to configure fan packages.]))
AC_ARG_ENABLE([presence],
    AS_HELP_STRING([--disable-presence], [Disable fan presence package.]))
AC_ARG_ENABLE([control],
    AS_HELP_STRING([--disable-control], [Disable fan control package.]))
AC_ARG_ENABLE([json-control],
    AS_HELP_STRING([--disable-json-control], [Disable the use of JSON to configure fan control in favor of YAML.]))
AC_ARG_ENABLE([cooling-type],
    AS_HELP_STRING([--disable-cooling-type], [Disable cooling-type package.]))
AC_ARG_ENABLE([monitor],
    AS_HELP_STRING([--disable-monitor], [Disable monitor]))
AC_ARG_ENABLE([sensor-monitor],
    AS_HELP_STRING([--enable-sensor-monitor], [Enable sensor monitor]))

AM_CONDITIONAL([WANT_JSON], [test "x$enable_json" == "xyes"])
AM_CONDITIONAL([WANT_JSON_CONTROL], [test "x$enable_json" == "xyes" -a "x$enable_json_control" != "xno"])
AM_CONDITIONAL([WANT_PRESENCE], [test "x$enable_presence" != "xno"])
AM_CONDITIONAL([WANT_CONTROL], [test "x$enable_control" != "xno"])
AM_CONDITIONAL([WANT_COOLING_TYPE], [test "x$enable_cooling_type" != "xno"])
AM_CONDITIONAL([WANT_MONITOR], [test "x$enable_monitor" != "xno"])
AM_CONDITIONAL([WANT_SENSOR_MONITOR], [test "x$enable_sensor_monitor" == "xyes"])

# Package specific checks.
AS_IF([test "x$enable_presence" != "xno"], [
    # Use runtime(json) config, otherwise default to compile time(yaml) config
    AM_COND_IF([WANT_JSON],
    [
        AC_ARG_VAR(MACHINE, [Machine name to install a particular set of associated config file(s)])
        AC_CHECK_HEADER(nlohmann/json.hpp, ,
                [AC_MSG_ERROR([Could not find nlohmann/json.hpp... nlohmann/json package required])])
        # Set config flag for runtime json usage
        AC_DEFINE([PRESENCE_USE_JSON], [1], [Fan presence use runtime json configuration])
        AC_MSG_NOTICE([Fan presence json configuration usage enabled])
        AC_CONFIG_FILES([presence/service_files/json/phosphor-fan-presence-tach@.service])
    ],
    [
        # Add optional yaml file argument
        AC_ARG_VAR(PRESENCE_CONFIG, [The fan presence config file.])

       AS_IF([test "x$PRESENCE_CONFIG" == "x"],
             [PRESENCE_CONFIG="\${top_srcdir}/presence/example/example.yaml"])

       AC_DEFINE_UNQUOTED([PRESENCE_CONFIG], ["$PRESENCE_CONFIG"],
                          [The fan presence config file.])

       AC_SUBST([PFPGEN],
                ["$PYTHON \${top_srcdir}/presence/pfpgen.py generate-cpp -i $PRESENCE_CONFIG"])

        AC_CONFIG_FILES([presence/service_files/yaml/phosphor-fan-presence-tach@.service])
    ])

    AC_ARG_VAR(NUM_PRESENCE_LOG_ENTRIES, [Maximum number of entries in the message log])
    AS_IF([test "x$NUM_PRESENCE_LOG_ENTRIES" == "x"], [NUM_PRESENCE_LOG_ENTRIES=50])
    AC_DEFINE_UNQUOTED([NUM_PRESENCE_LOG_ENTRIES], [$NUM_PRESENCE_LOG_ENTRIES],
                       [Maximum number of entries in the message log])

    AC_CONFIG_FILES([presence/Makefile])
])

AS_IF([test "x$enable_control" != "xno"], [
    # Add fan control Dbus attributes
    AC_ARG_VAR(CONTROL_BUSNAME, [The fan control busname to own])
    AS_IF([test "x$CONTROL_BUSNAME" == "x"],
          [CONTROL_BUSNAME="xyz.openbmc_project.Control.Thermal"])
    AC_DEFINE_UNQUOTED([CONTROL_BUSNAME], ["$CONTROL_BUSNAME"],
                       [The fan control busname to own])

    AC_ARG_VAR(CONTROL_OBJPATH, [The fan control root object path])
    AS_IF([test "x$CONTROL_OBJPATH" == "x"],
          [CONTROL_OBJPATH="/xyz/openbmc_project/control/thermal"])
    AC_DEFINE_UNQUOTED([CONTROL_OBJPATH], ["$CONTROL_OBJPATH"],
                       [The fan control root object path])

    AC_ARG_VAR(CONTROL_PERSIST_ROOT_PATH, [Root path for persisting zone property states])
    AS_IF([test "x$CONTROL_PERSIST_ROOT_PATH" == "x"],
          [CONTROL_PERSIST_ROOT_PATH="/var/lib/phosphor-fan-presence/control"])
    AC_DEFINE_UNQUOTED([CONTROL_PERSIST_ROOT_PATH], ["$CONTROL_PERSIST_ROOT_PATH"],
                       [Root path for persisting zone property states])

    # Use runtime(json) config, otherwise default to compile time(yaml) config
    AM_COND_IF([WANT_JSON_CONTROL],
    [
        AC_CHECK_HEADER(nlohmann/json.hpp, ,
                [AC_MSG_ERROR([Could not find nlohmann/json.hpp... nlohmann/json package required])])
        # Set config flag for runtime json usage
        AC_DEFINE([CONTROL_USE_JSON], [1], [Fan control use runtime json configuration])
        AC_MSG_NOTICE([Fan control json configuration usage enabled])
        AC_CONFIG_FILES([control/service_files/json/phosphor-fan-control@.service])
    ],
    [
        # Add optional yaml file arguments
        AC_ARG_VAR(FAN_DEF_YAML_FILE,
                   [The fan definition file to use])
        AS_IF([test "x$FAN_DEF_YAML_FILE" == "x"],
              [FAN_DEF_YAML_FILE="\${top_srcdir}/control/example/fans.yaml"])
        AC_DEFINE_UNQUOTED([FAN_DEF_YAML_FILE], ["$FAN_DEF_YAML_FILE"],
                           [The fan definition file to use])

        AC_ARG_VAR(FAN_ZONE_YAML_FILE,
                   [The fan zone definition file to use])
        AS_IF([test "x$FAN_ZONE_YAML_FILE" == "x"],
              [FAN_ZONE_YAML_FILE="\${top_srcdir}/control/example/zones.yaml"])
        AC_DEFINE_UNQUOTED([FAN_ZONE_YAML_FILE], ["$FAN_ZONE_YAML_FILE"],
                           [The fan zone definition file to use])

        AC_ARG_VAR(ZONE_EVENTS_YAML_FILE,
                   [The zone events definition file to use])
        AS_IF([test "x$ZONE_EVENTS_YAML_FILE" == "x"],
              [ZONE_EVENTS_YAML_FILE="\${top_srcdir}/control/example/events.yaml"])
        AC_DEFINE_UNQUOTED([ZONE_EVENTS_YAML_FILE], ["$ZONE_EVENTS_YAML_FILE"],
                           [The zone events definition file to use])

        AC_ARG_VAR(ZONE_CONDITIONS_YAML_FILE,
                   [The zone conditions definition file to use])
        AS_IF([test "x$ZONE_CONDITIONS_YAML_FILE" == "x"],
              [ZONE_CONDITIONS_YAML_FILE="\${top_srcdir}/control/example/zone_conditions.yaml"])
        AC_DEFINE_UNQUOTED([ZONE_CONDITIONS_YAML_FILE], ["$ZONE_CONDITIONS_YAML_FILE"],
                           [The zone conditions definition file to use])

        AC_SUBST([GEN_FAN_ZONE_DEFS],
                 ["$PYTHON \${top_srcdir}/control/gen-fan-zone-defs.py \
                      -f $FAN_DEF_YAML_FILE \
                      -z $FAN_ZONE_YAML_FILE \
                      -e $ZONE_EVENTS_YAML_FILE \
                      -c $ZONE_CONDITIONS_YAML_FILE"])

        AC_CONFIG_FILES([control/service_files/yaml/phosphor-fan-control-init@.service
                         control/service_files/yaml/phosphor-fan-control@.service])
    ])
    AC_CONFIG_FILES([control/Makefile])
])

AS_IF([test "x$enable_cooling_type" != "xno"], [
       AC_CONFIG_FILES([cooling-type/Makefile])
])

AS_IF([test "x$enable_monitor" != "xno"], [
    # Use runtime(json) config, otherwise default to compile time(yaml) config
    AM_COND_IF([WANT_JSON],
    [
        AC_CHECK_HEADER(nlohmann/json.hpp, ,
                [AC_MSG_ERROR([Could not find nlohmann/json.hpp... nlohmann/json package required])])
        # Set config flag for runtime json usage
        AC_DEFINE([MONITOR_USE_JSON], [1], [Fan monitor use runtime json configuration])
        AC_MSG_NOTICE([Fan monitor json configuration usage enabled])
        AC_CONFIG_FILES([monitor/service_files/json/phosphor-fan-monitor@.service])
    ],
    [
        AC_ARG_VAR(FAN_MONITOR_YAML_FILE,
                   [The fan monitor definition file to use])
        AS_IF([test "x$FAN_MONITOR_YAML_FILE" == "x"],
              [FAN_MONITOR_YAML_FILE="\${top_srcdir}/monitor/example/monitor.yaml"])
        AC_DEFINE_UNQUOTED([FAN_MONITOR_YAML_FILE], ["$FAN_MONITOR_YAML_FILE"],
                           [The fan monitor definition file to use])

        AC_SUBST([GEN_FAN_MONITOR_DEFS],
                 ["$PYTHON \${top_srcdir}/monitor/gen-fan-monitor-defs.py \
                  -m $FAN_MONITOR_YAML_FILE"])

        AC_CONFIG_FILES([monitor/service_files/yaml/phosphor-fan-monitor-init@.service
                         monitor/service_files/yaml/phosphor-fan-monitor@.service])
    ])

    AC_ARG_VAR(NUM_MONITOR_LOG_ENTRIES, [Maximum number of entries in the message log])
    AS_IF([test "x$NUM_MONITOR_LOG_ENTRIES" == "x"], [NUM_MONITOR_LOG_ENTRIES=75])
    AC_DEFINE_UNQUOTED([NUM_MONITOR_LOG_ENTRIES], [$NUM_MONITOR_LOG_ENTRIES],
                       [Maximum number of entries in the message log])

    AC_ARG_VAR(THERMAL_ALERT_BUSNAME, [The thermal alert busname to own])
    AS_IF([test "x$THERMAL_ALERT_BUSNAME" == "x"],
          [THERMAL_ALERT_BUSNAME="xyz.openbmc_project.Thermal.Alert"])
    AC_DEFINE_UNQUOTED([THERMAL_ALERT_BUSNAME], ["$THERMAL_ALERT_BUSNAME"],
                       [The thermal alert busname to own])

    AC_ARG_VAR(THERMAL_ALERT_OBJPATH, [The thermal alert D-Bus object path])
    AS_IF([test "x$THERMAL_ALERT_OBJPATH" == "x"],
          [THERMAL_ALERT_OBJPATH="/xyz/openbmc_project/alerts/thermal_fault_alert"])
    AC_DEFINE_UNQUOTED([THERMAL_ALERT_OBJPATH], ["$THERMAL_ALERT_OBJPATH"],
                       [The thermal alert D-Bus object path])

    AC_CONFIG_FILES([monitor/Makefile])
])

AS_IF([test "x$enable_sensor_monitor" == "xyes"], [

    AC_ARG_VAR(SENSOR_MONITOR_PERSIST_ROOT_PATH,
               [Root path for persisting sensor monitor data])
    AS_IF([test "x$SENSOR_MONITOR_PERSIST_ROOT_PATH" == "x"],
          [SENSOR_MONITOR_PERSIST_ROOT_PATH="/var/lib/phosphor-fan-presence/sensor-monitor"])
    AC_DEFINE_UNQUOTED([SENSOR_MONITOR_PERSIST_ROOT_PATH],
                       ["$SENSOR_MONITOR_PERSIST_ROOT_PATH"],
                       [Root path for persisting sensor monitor data])

    #Default hard shutdown delay is 23 seconds
    AC_ARG_VAR(SHUTDOWN_ALARM_HARD_SHUTDOWN_DELAY_MS,
            [Milliseconds to delay the alarm hard shutdown])
    AS_IF([test "x$SHUTDOWN_ALARM_HARD_SHUTDOWN_DELAY_MS" == "x"],
        [SHUTDOWN_ALARM_HARD_SHUTDOWN_DELAY_MS=23000])
    AC_DEFINE_UNQUOTED([SHUTDOWN_ALARM_HARD_SHUTDOWN_DELAY_MS],
                       [$SHUTDOWN_ALARM_HARD_SHUTDOWN_DELAY_MS],
                       [Milliseconds to delay the alarm hard shutdown])

    #Default soft shutdown delay is 15 minutes
    AC_ARG_VAR(SHUTDOWN_ALARM_SOFT_SHUTDOWN_DELAY_MS,
               [Milliseconds to delay the alarm soft shutdown])
    AS_IF([test "x$SHUTDOWN_ALARM_SOFT_SHUTDOWN_DELAY_MS" == "x"],
        [SHUTDOWN_ALARM_SOFT_SHUTDOWN_DELAY_MS=900000])
    AC_DEFINE_UNQUOTED([SHUTDOWN_ALARM_SOFT_SHUTDOWN_DELAY_MS],
                       [$SHUTDOWN_ALARM_SOFT_SHUTDOWN_DELAY_MS],
                       [Milliseconds to delay the alarm soft shutdown])

    AC_CONFIG_FILES([sensor-monitor/Makefile sensor-monitor/service_files/sensor-monitor.service])
])

eval "eval BINDIR=$bindir"
AC_SUBST([BINDIR])


# Create configured output
AC_CONFIG_FILES([Makefile test/Makefile presence/test/Makefile monitor/test/Makefile])
AC_OUTPUT
