AM_DEFAULT_SOURCE_EXT = .cpp

libfan_sensor_handlerdir = ${libdir}/phosphor-fan
libfan_sensor_handler_LTLIBRARIES = libfan_sensor_handler.la
libfan_sensor_handler_la_SOURCES = \
    sensor_handler.cpp \
    xyz.openbmc_project.Fan.Sensor.cpp
libfan_sensor_handler_la_LDFLAGS = $(SYSTEMD_LIBS) -version-info 0:0:0 -shared
libfan_sensor_handler_la_CXXFLAGS = $(SYSTEMD_CFLAGS)

sbin_PROGRAMS = \
    phosphor-fan-tach-feedback-presence

phosphor_fan_tach_feedback_presence_SOURCES = \
    tach_feedback.cpp \
    xyz.openbmc_project.Fan.Sensor.cpp # Linking error results without this?
# TODO Make this path configurable
phosphor_fan_tach_feedback_presence_CPPFLAGS = \
    -DFAN_SENSOR_LIB_PATH=\"/usr/lib/phosphor-fan/\"
phosphor_fan_tach_feedback_presence_LDFLAGS = $(SYSTEMD_LIBS)
