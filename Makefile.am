AM_DEFAULT_SOURCE_EXT = .cpp

sbin_PROGRAMS = \
	phosphor-fan-presence-tach

phosphor_fan_presence_tach_SOURCES = \
	fan_enclosure.cpp \
	tach_sensor.cpp \
	tach_detect.cpp
nodist_phosphor_fan_presence_tach_SOURCES = \
	fan_detect_defs.cpp

phosphor_fan_presence_tach_LDFLAGS = $(SDBUSPLUS_LIBS) $(PHOSPHOR_LOGGING_LIBS)
phosphor_fan_presence_tach_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS)

BUILT_SOURCES = fan_detect_defs.cpp
CLEANFILES = fan_detect_defs.cpp

fan_detect_defs.cpp: ${srcdir}/gen-fan-detect-defs.py
	$(AM_V_GEN)$(GEN_FAN_DETECT_DEFS) > $@
