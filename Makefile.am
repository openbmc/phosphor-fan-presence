AM_DEFAULT_SOURCE_EXT = .cpp

sbin_PROGRAMS = \
	phosphor-fan-presence-tach

phosphor_fan_presence_tach_SOURCES = \
	fan_enclosure.cpp \
	tach_sensor.cpp \
	tach_detect.cpp

phosphor_fan_presence_tach_LDFLAGS = $(SDBUSPLUS_LIBS)
phosphor_fan_presence_tach_CXXFLAGS = $(SDBUSPLUS_CFLAGS)

BUILT_SOURCES = fan_detect_defs.hpp
CLEANFILES = fan_detect_defs.hpp

fan_detect_defs.hpp: ${srcdir}/gen-fan-detect-defs.py
	$(AM_V_GEN)$(GEN_FAN_DETECT_DEFS) > $@
