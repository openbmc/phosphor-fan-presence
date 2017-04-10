AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -iquote ${top_srcdir}

sbin_PROGRAMS = \
	phosphor-fan-control

phosphor_fan_control_SOURCES = \
	generated.cpp \
	fan.cpp \
	main.cpp \
	manager.cpp \
	zone.cpp

phosphor_fan_control_LDADD = \
	$(top_builddir)/libfan.la \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS)

phosphor_fan_control_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS)
