AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -iquote ${top_srcdir}

sbin_PROGRAMS = \
	phosphor-fan-monitor

phosphor_fan_monitor_SOURCES = \
	fan.cpp \
	main.cpp \
	tach_sensor.cpp

nodist_phosphor_fan_monitor_SOURCES = \
	fan_monitor_defs.cpp

BUILT_SOURCES = fan_monitor_defs.cpp

phosphor_fan_monitor_LDADD = \
	$(top_builddir)/libfan.la \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	${PHOSPHOR_DBUS_INTERFACES_LIBS}

phosphor_fan_monitor_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	${PHOSPHOR_DBUS_INTERFACES_CFLAGS}

fan_monitor_defs.cpp: ${srcdir}/gen-fan-monitor-defs.py
	$(AM_V_GEN)$(GEN_FAN_MONITOR_DEFS)
