AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -iquote ${top_srcdir}

sbin_PROGRAMS = \
	phosphor-fan-control

phosphor_fan_control_SOURCES = \
	argument.cpp \
	fan.cpp \
	main.cpp \
	manager.cpp \
	actions.cpp \
	zone.cpp

nodist_phosphor_fan_control_SOURCES = \
	fan_zone_defs.cpp \
	xyz/openbmc_project/Control/Thermal/server.cpp

phosphor_fan_control_LDADD = \
	-lstdc++fs \
	$(top_builddir)/libfan.la \
	$(SDBUSPLUS_LIBS) \
	$(SDEVENTPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	${PHOSPHOR_DBUS_INTERFACES_LIBS}

phosphor_fan_control_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(SDEVENTPLUS_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	${PHOSPHOR_DBUS_INTERFACES_CFLAGS} \
	-flto

BUILT_SOURCES = fan_zone_defs.cpp \
				xyz/openbmc_project/Control/Thermal/server.cpp \
				xyz/openbmc_project/Control/Thermal/server.hpp

CLEANFILES = xyz/openbmc_project/Control/Thermal/server.cpp \
			 xyz/openbmc_project/Control/Thermal/server.hpp

xyz/openbmc_project/Control/Thermal/server.cpp: xyz/openbmc_project/Control/Thermal.interface.yaml
	@mkdir -p `dirname $@`
	$(SDBUSPLUSPLUS) -r $(srcdir) interface server-cpp xyz.openbmc_project.Control.Thermal > $@

xyz/openbmc_project/Control/Thermal/server.hpp: xyz/openbmc_project/Control/Thermal.interface.yaml
	@mkdir -p `dirname $@`
	$(SDBUSPLUSPLUS) -r $(srcdir) interface server-header xyz.openbmc_project.Control.Thermal > $@

fan_zone_defs.cpp: ${srcdir}/gen-fan-zone-defs.py
	$(AM_V_GEN)$(GEN_FAN_ZONE_DEFS)
