AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -iquote ${top_srcdir}

bin_PROGRAMS = \
	phosphor-fan-monitor

systemdsystemunit_DATA=

if WANT_JSON
systemdsystemunit_DATA += \
	service_files/json/phosphor-fan-monitor@.service
else
systemdsystemunit_DATA += \
	service_files/yaml/phosphor-fan-monitor@.service \
	service_files/yaml/phosphor-fan-monitor-init@.service
endif

phosphor_fan_monitor_SOURCES = \
	argument.cpp \
	fan.cpp \
	fan_error.cpp \
	power_interface.cpp \
	logging.cpp \
	main.cpp \
	tach_sensor.cpp \
	conditions.cpp \
	system.cpp

phosphor_fan_monitor_LDADD = \
	$(SDBUSPLUS_LIBS) \
	$(SDEVENTPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	${PHOSPHOR_DBUS_INTERFACES_LIBS} \
	-lstdc++fs \
	$(STDPLUS_LIBS) \
	$(FMT_LIBS)

phosphor_fan_monitor_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(SDEVENTPLUS_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	${PHOSPHOR_DBUS_INTERFACES_CFLAGS} \
	-flto \
	$(STDPLUS_CFLAGS)

if WANT_JSON
phosphor_fan_monitor_SOURCES += json_parser.cpp
else
BUILT_SOURCES = fan_monitor_defs.cpp
nodist_phosphor_fan_monitor_SOURCES = \
	fan_monitor_defs.cpp

fan_monitor_defs.cpp: ${srcdir}/gen-fan-monitor-defs.py
	$(AM_V_GEN)$(GEN_FAN_MONITOR_DEFS) > ${builddir}/$@
endif

SUBDIRS = test
