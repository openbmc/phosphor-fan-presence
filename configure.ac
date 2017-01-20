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

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_14([noext])
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CXXFLAGS])

# Checks for libraries.
PKG_CHECK_MODULES([SYSTEMD], [libsystemd >= 221], ,
[AC_MSG_ERROR([The systemd development package is required])])

# Checks for header files.
AC_CHECK_HEADER(sdbusplus/bus.hpp, ,
[AC_MSG_ERROR([Could not find sdbusplus/bus.hpp...openbmc/sdbusplus required])])

# Checks for library functions.
LT_INIT # Required for systemd linking

# Set dbus parameters
AC_ARG_VAR(FAN_SENSOR_VALUE_INTERF, [Fan sensor value interface])
AS_IF([test "x$FAN_SENSOR_VALUE_INTERF" == "x"],
[FAN_SENSOR_VALUE_INTERF="xyz.openbmc_project.sensors.value"])
AC_DEFINE_UNQUOTED([FAN_SENSOR_VALUE_INTERF], ["$FAN_SENSOR_VALUE_INTERF"],
[Fan sensor value interface])

# Create configured output
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
