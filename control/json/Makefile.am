AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -I${top_srcdir} -I${top_srcdir}/control

noinst_LTLIBRARIES = libfan-control-json.la
libfan_control_json_la_LDFLAGS = -static
libfan_control_json_la_LIBADD = \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS)

libfan_control_json_la_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS)

libfan_control_json_la_SOURCES = \
	manager.cpp \
	profiles.cpp
