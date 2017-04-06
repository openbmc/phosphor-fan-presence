AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -I${top_srcdir}
#BUILT_SOURCES = $(top_builddir)/utilities.cpp
#BUILT_SOURCES = $(top_builddir)/utilities.o

sbin_PROGRAMS = \
	phosphor-cooling-type

# Source file with main() should be first.
# Using .. instead of $(top_srcdir) due to automake bug in version 1.15.
#  https://debbugs.gnu.org/cgi/bugreport.cgi?bug=13928
phosphor_cooling_type_SOURCES = \
	cooling_type.cpp \
	argument.cpp \
	../utility.cpp
phosphor_cooling_type_CXXFLAGS = -I${top_srcdir} $(SDBUSPLUS_CFLAGS) $(PHOSPHOR_LOGGING_CFLAGS)
phosphor_cooling_type_LDFLAGS = $(SDBUSPLUS_LIBS) $(PHOSPHOR_LOGGING_LIBS)

