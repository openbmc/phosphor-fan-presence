AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -iquote ${top_srcdir}

sbin_PROGRAMS = \
	phosphor-fan-monitor

phosphor_fan_monitor_SOURCES = \
	fan.cpp \
	generated.cpp \
	main.cpp \
	tach_sensor.cpp

phosphor_fan_monitor_LDADD = \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS)

phosphor_fan_monitor_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS)

