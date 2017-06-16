AM_DEFAULT_SOURCE_EXT = .cpp

noinst_LTLIBRARIES = libfan.la
libfan_la_LDFLAGS = -static
libfan_la_LIBADD = \
	$(SDBUSPLUS_LIBS)
libfan_la_CXXFLAGS =
	$(SDBUSPLUS_CFLAGS)
libfan_la_SOURCES = \
	utility.cpp \
	timer.cpp

SUBDIRS = . test sdevent/test evdevpp/test

if WANT_PRESENCE
SUBDIRS += presence
endif
if WANT_CONTROL
SUBDIRS += control
endif
if WANT_COOLING_TYPE
SUBDIRS += cooling-type
endif
if WANT_MONITOR
SUBDIRS += monitor
endif
