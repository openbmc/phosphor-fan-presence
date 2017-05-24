AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -iquote ${top_srcdir}

sbin_PROGRAMS = \
	phosphor-set-gpio

phosphor_set_gpio_SOURCES = \
	gpio.cpp \
	main.cpp

phosphor_set_gpio_LDADD = \
	$(PHOSPHOR_LOGGING_LIBS)

phosphor_set_gpio_CXXFLAGS = \
	$(PHOSPHOR_LOGGING_CFLAGS)
