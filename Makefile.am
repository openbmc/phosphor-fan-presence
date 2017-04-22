AM_DEFAULT_SOURCE_EXT = .cpp

noinst_LTLIBRARIES = libfan.la
libfan_la_LDFLAGS = -static
libfan_la_LIBADD = \
	$(SDBUSPLUS_LIBS)
libfan_la_CXXFLAGS =
	$(SDBUSPLUS_CFLAGS)
libfan_la_SOURCES = \
	utility.cpp

SUBDIRS = . presence control
