AM_DEFAULT_SOURCE_EXT = .cpp

libfan_sensor_handlerdir = ${libdir}
libfan_sensor_handler_LTLIBRARIES = libfan_sensor_handler.la
libfan_sensor_handler_la_SOURCES = sensor_handler.cpp
libfan_sensor_handler_la_LDFLAGS = $(SYSTEMD_LIBS) -version-info 0:0:0 -shared
libfan_sensor_handler_la_CXXFLAGS = $(SYSTEMD_CFLAGS)
