AM_CPPFLAGS = -I$(top_srcdir)

# Run all 'check' test programs
TESTS = $(check_PROGRAMS)

# # Build/add timertest to test suite
check_PROGRAMS = timertest
timertest_CPPFLAGS = -Igtest $(GTEST_CPPFLAGS) $(AM_CPPFLAGS)
timertest_CXXFLAGS = $(PTHREAD_CFLAGS) ${PHOSPHOR_DBUS_INTERFACES_CFLAGS}
timertest_LDFLAGS = -lgtest_main -lgtest $(PTHREAD_LIBS) $(OESDK_TESTCASE_FLAGS) \
                    $(SYSTEMD_LIBS) ${SDBUSPLUS_LIBS} ${PHOSPHOR_DBUS_INTERFACES_LIBS}
timertest_SOURCES = timertest.cpp
timertest_LDADD = $(top_builddir)/timer.o
