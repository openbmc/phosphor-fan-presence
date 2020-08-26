AM_CPPFLAGS = -iquote$(top_srcdir)/control
gtest_cflags = $(PTHREAD_CFLAGS)
gtest_ldadd = -lgtest -lgtest_main -lgmock $(PTHREAD_LIBS)

check_PROGRAMS = profile_test

TESTS = $(check_PROGRAMS)

profile_test_SOURCES = \
	profile_test.cpp
profile_test_CXXFLAGS = \
	$(gtest_cflags) \
	${PHOSPHOR_DBUS_INTERFACES_CFLAGS} \
	$(SDBUSPLUS_CFLAGS)
profile_test_LDFLAGS = \
	$(OESDK_TESTCASE_FLAGS)
profile_test_LDADD = \
	${builddir}/../profile.o \
	$(gtest_ldadd) \
	${PHOSPHOR_DBUS_INTERFACES_LIBS} \
	$(SDBUSPLUS_LIBS)
