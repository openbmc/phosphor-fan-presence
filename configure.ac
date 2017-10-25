# Initialization
AC_PREREQ([2.69])
AC_INIT([phosphor-fan-presence], [1.0],
[https://github.com/openbmc/phosphor-fan-presence/issues])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CXX
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Python
AM_PATH_PYTHON([2.7], [AC_SUBST([PYTHON], [echo "$PYTHON"])],
[AC_MSG_ERROR([Could not find python-2.7 installed...python-2.7 is required])])

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_14([noext])
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CXXFLAGS])

# Checks for libraries.
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus], ,
[AC_MSG_ERROR([The openbmc/sdbusplus package is required])])
PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging], ,
[AC_MSG_ERROR([The openbmc/phosphor-logging package is required])])
PKG_CHECK_MODULES([SYSTEMD], [libsystemd >= 221], ,
[AC_MSG_ERROR([Could not find systemd...systemd development package required])])
PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [phosphor-dbus-interfaces], ,
[AC_MSG_ERROR([Could not find phosphor-dbus-interfaces...openbmc/phosphor-dbus-interfaces package required])])
PKG_CHECK_MODULES([LIBEVDEV], [libevdev], ,
[AC_MSG_ERROR([The libevdev package is required])])

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

AC_ARG_ENABLE([presence],
    AS_HELP_STRING([--disable-presence], [Disable fan presence package.]))
AC_ARG_ENABLE([control],
    AS_HELP_STRING([--disable-control], [Disable fan control package.]))
AC_ARG_ENABLE([cooling-type],
    AS_HELP_STRING([--disable-cooling-type], [Disable cooling-type package.]))
AC_ARG_ENABLE([monitor],
    AS_HELP_STRING([--disable-monitor], [Disable monitor]))

AM_CONDITIONAL([WANT_PRESENCE], [test "x$enable_presence" != "xno"])
AM_CONDITIONAL([WANT_CONTROL], [test "x$enable_control" != "xno"])
AM_CONDITIONAL([WANT_COOLING_TYPE], [test "x$enable_cooling_type" != "xno"])
AM_CONDITIONAL([WANT_MONITOR], [test "x$enable_monitor" != "xno"])

# Package specific checks.
AS_IF([test "x$enable_presence" != "xno"], [
       # Add optional yaml file argument
       AC_ARG_VAR(PRESENCE_CONFIG,
                  [The fan presence config file.])

       AS_IF([test "x$PRESENCE_CONFIG" == "x"],
             [PRESENCE_CONFIG="\${top_srcdir}/presence/example/example.yaml"])

       AC_DEFINE_UNQUOTED([PRESENCE_CONFIG], ["$PRESENCE_CONFIG"],
                          [The fan presence config file.])

       AC_SUBST([PFPGEN],
                ["$PYTHON \${top_srcdir}/presence/pfpgen.py generate-cpp -i $PRESENCE_CONFIG"])

       AC_CONFIG_FILES([presence/Makefile])
       ])

AS_IF([test "x$enable_control" != "xno"], [
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

       AC_ARG_VAR(FAN_ZONE_OUTPUT_DIR,
                  [The output directory for the generated fan zone data file])
       AS_IF([test "x$FAN_ZONE_OUTPUT_DIR" == "x"],
             [FAN_ZONE_OUTPUT_DIR="\${top_srcdir}/control/"])
       AC_DEFINE_UNQUOTED([FAN_ZONE_OUTPUT_DIR], ["$FAN_ZONE_OUTPUT_DIR"],
                          [The output directory for the generated fan zone data file])

       AC_SUBST([GEN_FAN_ZONE_DEFS],
                ["$PYTHON \${top_srcdir}/control/gen-fan-zone-defs.py \
                         -f $FAN_DEF_YAML_FILE \
                         -z $FAN_ZONE_YAML_FILE \
                         -e $ZONE_EVENTS_YAML_FILE \
                         -c $ZONE_CONDITIONS_YAML_FILE \
                         -o $FAN_ZONE_OUTPUT_DIR"])
       AC_CONFIG_FILES([control/Makefile])
])

AS_IF([test "x$enable_cooling_type" != "xno"], [
       AC_CONFIG_FILES([cooling-type/Makefile])
])
AS_IF([test "x$enable_monitor" != "xno"], [

       AC_ARG_VAR(FAN_MONITOR_YAML_FILE,
                  [The fan monitor definition file to use])
       AS_IF([test "x$FAN_MONITOR_YAML_FILE" == "x"],
             [FAN_MONITOR_YAML_FILE="${srcdir}/monitor/example/monitor.yaml"])
       AC_DEFINE_UNQUOTED([FAN_MONITOR_YAML_FILE], ["$FAN_MONITOR_YAML_FILE"],
                          [The fan monitor definition file to use])

       AC_ARG_VAR(FAN_MONITOR_OUTPUT_DIR,
                  [The output directory for the generated fan monitor data file])
       AS_IF([test "x$FAN_MONITOR_OUTPUT_DIR" == "x"],
             [FAN_MONITOR_OUTPUT_DIR="${srcdir}/monitor/"])
       AC_DEFINE_UNQUOTED([FAN_MONITOR_OUTPUT_DIR], ["$FAN_MONITOR_OUTPUT_DIR"],
                          [The output directory for the generated fan monitor data file])

       AC_SUBST([GEN_FAN_MONITOR_DEFS],
                [$PYTHON ${srcdir}/monitor/gen-fan-monitor-defs.py \
                         -m $FAN_MONITOR_YAML_FILE \
                         -o $FAN_MONITOR_OUTPUT_DIR])

       AC_CONFIG_FILES([monitor/Makefile])
])

# Create configured output
AC_CONFIG_FILES([Makefile test/Makefile sdevent/test/Makefile evdevpp/test/Makefile presence/test/Makefile])
AC_OUTPUT
