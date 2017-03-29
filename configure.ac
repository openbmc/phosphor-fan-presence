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

# Checks for header files.

# Checks for library functions.
LT_INIT # Required for systemd linking

# Add optional yaml file argument
AC_ARG_VAR(FAN_DETECT_YAML_FILE,
[The fan presence detection definition file to use])
AS_IF([test "x$FAN_DETECT_YAML_FILE" == "x"],
[FAN_DETECT_YAML_FILE="${srcdir}/example/fan-detect.yaml"])
AC_DEFINE_UNQUOTED([FAN_DETECT_YAML_FILE], ["$FAN_DETECT_YAML_FILE"],
[The fan presence detection definition file to use])

# Create configured output
AC_SUBST([GEN_FAN_DETECT_DEFS],
[$PYTHON ${srcdir}/gen-fan-detect-defs.py -y $FAN_DETECT_YAML_FILE])
AC_CONFIG_FILES([Makefile chassis-cooling-type/Makefile])
AC_OUTPUT
