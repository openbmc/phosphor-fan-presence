AM_CPPFLAGS = -iquote$(top_srcdir)
gtest_cflags = $(PTHREAD_CFLAGS)
gtest_ldadd = -lgtest -lgtest_main -lgmock $(PTHREAD_LIBS)

check_PROGRAMS = logger_test

TESTS = $(check_PROGRAMS)

logger_test_SOURCES = \
	logger_test.cpp
logger_test_CXXFLAGS = \
	$(gtest_cflags) \
	${PHOSPHOR_DBUS_INTERFACES_CFLAGS} \
	$(SDBUSPLUS_CFLAGS)
logger_test_LDFLAGS = \
	$(OESDK_TESTCASE_FLAGS)
logger_test_LDADD = \
	$(gtest_ldadd) \
	${PHOSPHOR_DBUS_INTERFACES_LIBS} \
	$(SDBUSPLUS_LIBS) \
	$(FMT_LIBS)
