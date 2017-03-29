AM_DEFAULT_SOURCE_EXT = .cpp

noinst_LTLIBRARIES = libfan.la
libfan_la_LDFLAGS = -static
libfan_la_LIBADD = \
	$(SBUSPLUS_LIBS)
libfan_la_CXXFLAGS =
	$(SBUSPLUS_XFLAGS)
libfan_la_SOURCES = \
	utility.cpp

SUBDIRS = .

if WANT_PRESENCE
SUBDIRS += presence
endif
if WANT_CONTROL
SUBDIRS += control
endif
if WANT_COOLING_TYPE
SUBDIRS += chassis-cooling-type
endif
