AM_DEFAULT_SOURCE_EXT = .cpp

sbin_PROGRAMS = \
	phosphor-fan-presence-tach

phosphor_fan_presence_tach_SOURCES = \
	fan-enclosure.cpp \
	tach-sensor.cpp \
	tach-detect.cpp

phosphor_fan_presence_tach_LDFLAGS = $(SYSTEMD_LIBS)

BUILT_SOURCES = fan-detect-defs.hpp
CLEANFILES = fan-detect-defs.hpp

fan-detect-defs.hpp: ${srcdir}/gen-fan-detect-defs.py
	$(AM_V_at)$(GEN_FAN_DETECT_DEFS) > $@
