AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -iquote ${top_srcdir}

bin_PROGRAMS = \
	sensor-monitor

systemdsystemunit_DATA = \
	service_files/sensor-monitor.service

sensor_monitor_SOURCES = \
    shutdown_alarm_monitor.cpp \
    threshold_alarm_logger.cpp \
	main.cpp

sensor_monitor_LDADD = \
	$(SDBUSPLUS_LIBS) \
	$(SDEVENTPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	${PHOSPHOR_DBUS_INTERFACES_LIBS} \
	-lstdc++fs \
	$(FMT_LIBS)

sensor_monitor_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(SDEVENTPLUS_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	${PHOSPHOR_DBUS_INTERFACES_CFLAGS} \
	-flto

if HOST_STATE_ENABLED
ENABLE_HOST_STATE = 1
sensor_monitor_CXXFLAGS+=-DENABLE_HOST_STATE
endif
