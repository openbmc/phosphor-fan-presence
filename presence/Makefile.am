AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -iquote ${top_srcdir}

bin_PROGRAMS = \
	phosphor-fan-presence-tach

systemdsystemunit_DATA=

if WANT_JSON
systemdsystemunit_DATA += \
	service_files/json/phosphor-fan-presence-tach@.service
else
systemdsystemunit_DATA += \
	service_files/yaml/phosphor-fan-presence-tach@.service
endif

phosphor_fan_presence_tach_SOURCES = \
	anyof.cpp \
	error_reporter.cpp \
	fallback.cpp \
	fan.cpp \
	gpio.cpp \
	logging.cpp \
	psensor.cpp \
	tach.cpp \
	tach_detect.cpp \
	json_parser.cpp

phosphor_fan_presence_tach_LDADD = \
	$(SDBUSPLUS_LIBS) \
	$(SDEVENTPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	${PHOSPHOR_DBUS_INTERFACES_LIBS} \
	$(LIBEVDEV_LIBS) \
	$(STDPLUS_LIBS) \
	$(FMT_LIBS)

phosphor_fan_presence_tach_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(SDEVENTPLUS_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	${PHOSPHOR_DBUS_INTERFACES_CFLAGS} \
	$(LIBEVDEV_CFLAGS) \
	-flto \
	$(STDPLUS_CFLAGS)

if !WANT_JSON
BUILT_SOURCES = generated.hpp
TEMPLATES = \
	templates/generated.mako.hpp

generated.hpp: $(TEMPLATES) ${srcdir}/pfpgen.py $(PRESENCE_CONFIG)
	$(AM_V_GEN)$(PFPGEN) > $(builddir)/$@
endif
