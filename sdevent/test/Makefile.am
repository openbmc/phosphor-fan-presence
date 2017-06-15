AM_CPPFLAGS = -iquote$(top_srcdir)
gtest_cflags = $(PTHREAD_CFLAGS)
gtest_ldadd = -lgtest -lgtest_main -lgmock $(PTHREAD_LIBS)

check_PROGRAMS =

TESTS = $(check_PROGRAMS)

check_PROGRAMS += iotest
iotest_SOURCES = \
	iotest.cpp
iotest_CXXFLAGS = \
	$(gtest_cflags) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(SDBUSPLUS_CFLAGS) \
	$(SYSTEMD_CFLAGS)
iotest_LDFLAGS = \
	$(OESDK_TESTCASE_FLAGS)
iotest_LDADD = \
	$(gtest_ldadd) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(SDBUSPLUS_LIBS) \
	$(SYSTEMD_LIBS)
