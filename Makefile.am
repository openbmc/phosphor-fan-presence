AM_DEFAULT_SOURCE_EXT = .cpp

noinst_LTLIBRARIES = libfan.la
libfan_la_LDFLAGS = -static
libfan_la_LIBADD = \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(SDBUSPLUS_LIBS)
libfan_la_CXXFLAGS = \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(SDBUSPLUS_CFLAGS) \
	-flto
libfan_la_SOURCES = \
	utility.cpp

SUBDIRS = . sdevent/test evdevpp/test

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
