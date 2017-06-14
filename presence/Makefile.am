AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -iquote ${top_srcdir}

sbin_PROGRAMS = \
	phosphor-fan-presence-tach

phosphor_fan_presence_tach_SOURCES = \
	fan_enclosure.cpp \
	tach_sensor.cpp \
	tach_detect.cpp
nodist_phosphor_fan_presence_tach_SOURCES = \
	fan_detect_defs.cpp

phosphor_fan_presence_tach_LDADD = \
	$(top_builddir)/libfan.la \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	${PHOSPHOR_DBUS_INTERFACES_LIBS}
phosphor_fan_presence_tach_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	${PHOSPHOR_DBUS_INTERFACES_CFLAGS}

BUILT_SOURCES = fan_detect_defs.cpp

fan_detect_defs.cpp: ${srcdir}/gen-fan-detect-defs.py $(FAN_DETECT_YAML_FILE)
	$(AM_V_GEN)$(GEN_FAN_DETECT_DEFS) > $(builddir)/$@
