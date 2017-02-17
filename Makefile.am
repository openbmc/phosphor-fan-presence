AM_DEFAULT_SOURCE_EXT = .cpp

sbin_PROGRAMS = \
	phosphor-fan-presence-tach

phosphor_fan_presence_tach_SOURCES = \
	fan_enclosure.cpp \
	tach_sensor.cpp \
	tach_detect.cpp

phosphor_fan_presence_tach_LDFLAGS = $(SDBUSPLUS_LIBS)
phosphor_fan_presence_tach_CXXFLAGS = $(SDBUSPLUS_CFLAGS)
