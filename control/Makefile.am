AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -iquote ${top_srcdir} \
	-I${srcdir}/json \
	-I${srcdir}/json/actions \
	-I${srcdir}/json/triggers

systemdsystemunit_DATA=

if WANT_JSON_CONTROL
systemdsystemunit_DATA += \
	service_files/json/phosphor-fan-control@.service
else
systemdsystemunit_DATA += \
	service_files/yaml/phosphor-fan-control@.service \
	service_files/yaml/phosphor-fan-control-init@.service
endif

bin_PROGRAMS = \
	phosphor-fan-control \
	fanctl

phosphor_fan_control_SOURCES = \
	main.cpp

phosphor_fan_control_LDADD = \
	-lstdc++fs \
	$(SDBUSPLUS_LIBS) \
	$(SDEVENTPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	${PHOSPHOR_DBUS_INTERFACES_LIBS} \
	$(FMT_LIBS)

phosphor_fan_control_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(SDEVENTPLUS_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	${PHOSPHOR_DBUS_INTERFACES_CFLAGS} \
	-flto

fanctl_SOURCES = \
	fanctl.cpp

fanctl_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	${PHOSPHOR_DBUS_INTERFACES_CFLAGS} \
	-flto

fanctl_LDADD = \
	-lstdc++fs \
	$(SDBUSPLUS_LIBS) \
	${PHOSPHOR_DBUS_INTERFACES_LIBS} \
	$(FMT_LIBS)


if WANT_JSON_CONTROL
phosphor_fan_control_SOURCES += \
	json/manager.cpp \
	json/profile.cpp \
	json/fan.cpp \
	json/zone.cpp \
	json/dbus_zone.cpp \
	json/group.cpp \
	json/event.cpp \
	json/triggers/timer.cpp \
	json/triggers/signal.cpp \
	json/triggers/init.cpp \
	json/actions/default_floor.cpp \
	json/actions/request_target_base.cpp \
	json/actions/missing_owner_target.cpp \
	json/actions/count_state_target.cpp \
	json/actions/net_target_increase.cpp \
	json/actions/net_target_decrease.cpp \
	json/actions/timer_based_actions.cpp
else
phosphor_fan_control_SOURCES += \
	argument.cpp \
	fan.cpp \
	manager.cpp \
	utility.cpp \
	preconditions.cpp \
	actions.cpp \
	triggers.cpp \
	zone.cpp
BUILT_SOURCES = fan_zone_defs.cpp
nodist_phosphor_fan_control_SOURCES = \
	fan_zone_defs.cpp

fan_zone_defs.cpp: ${srcdir}/gen-fan-zone-defs.py
	$(AM_V_GEN)$(GEN_FAN_ZONE_DEFS) > ${builddir}/$@
endif
