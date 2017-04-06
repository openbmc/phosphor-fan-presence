AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -iquote ${top_srcdir}

sbin_PROGRAMS = \
	phosphor-cooling-type

# Source file with main() should be first.
# Using .. instead of $(top_srcdir) due to automake bug in version 1.15.
#  https://debbugs.gnu.org/cgi/bugreport.cgi?bug=13928
phosphor_cooling_type_SOURCES = \
  main.cpp \
	cooling_type.cpp \
	argument.cpp
phosphor_cooling_type_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(LIBEVDEV_CFLAGS)
phosphor_cooling_type_LDADD = \
	${top_builddir}/libfan.la \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(LIBEVDEV_LIBS)

# vim: tabstop=8 noexpandtab
