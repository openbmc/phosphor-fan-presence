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

AM_CONDITIONAL([WANT_PRESENCE], [test "x$enable_presence" != "xno"])
AM_CONDITIONAL([WANT_CONTROL], [test "x$enable_control" != "xno"])
AM_CONDITIONAL([WANT_COOLING_TYPE], [test "x$enable_cooling-type" != "xno"])
AM_CONDITIONAL([WANT_MONITOR], [test "x$enable_monitor" != "xno"])

# Package specific checks.
AS_IF([test "x$enable_presence" != "xno"], [
       # Add optional yaml file argument
       AC_ARG_VAR(FAN_DETECT_YAML_FILE,
                  [The fan presence detection definition file to use])
       AS_IF([test "x$FAN_DETECT_YAML_FILE" == "x"],
             [FAN_DETECT_YAML_FILE="${srcdir}/presence/example/fan-detect.yaml"])
       AC_DEFINE_UNQUOTED([FAN_DETECT_YAML_FILE], ["$FAN_DETECT_YAML_FILE"],
                          [The fan presence detection definition file to use])

       AC_SUBST([GEN_FAN_DETECT_DEFS],
                [$PYTHON ${srcdir}/presence/gen-fan-detect-defs.py -y $FAN_DETECT_YAML_FILE])
       AC_CONFIG_FILES([presence/Makefile])
       ])

AS_IF([test "x$enable_control" != "xno"], [
       AC_CONFIG_FILES([control/Makefile])
])

AS_IF([test "x$enable_cooling-type" != "xno"], [
       PKG_CHECK_MODULES([LIBEVDEV], [libevdev], ,
                         [AC_MSG_ERROR([The libevdev package is required])])
       AC_CONFIG_FILES([chassis-cooling-type/Makefile])
])
AS_IF([test "x$enable_monitor" != "xno"], [
       AC_CONFIG_FILES([monitor/Makefile])
])

# Create configured output
AC_CONFIG_FILES([Makefile test/Makefile])
AC_OUTPUT
