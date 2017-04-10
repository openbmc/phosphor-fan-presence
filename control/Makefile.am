AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -I${top_src_dir}

sbin_PROGRAMS = \
	phosphor-fan-control

phosphor_fan_control_SOURCES = \
	main.cpp

phosphor_fan_control_LDFLAGS = $(SDBUSPLUS_LIBS) $(PHOSPHOR_LOGGING_LIBS)
phosphor_fan_control_CXXFLAGS = $(SDBUSPLUS_CFLAGS) $(PHOSPHOR_LOGGING_CFLAGS)
