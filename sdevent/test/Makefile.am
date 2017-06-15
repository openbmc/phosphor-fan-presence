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
	$(SYSTEMD_CFLAGS)
iotest_LDFLAGS = \
	$(OESDK_TESTCASE_FLAGS)
iotest_LDADD = \
	$(gtest_ldadd) \
	$(SYSTEMD_LIBS)
