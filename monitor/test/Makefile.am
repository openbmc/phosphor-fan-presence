AM_CPPFLAGS = -iquote$(top_srcdir)
gtest_cflags = $(PTHREAD_CFLAGS)
gtest_ldadd = -lgtest -lgtest_main -lgmock $(PTHREAD_LIBS)

check_PROGRAMS =

TESTS = $(check_PROGRAMS)

check_PROGRAMS += \
	power_off_cause_test \
	power_off_rule_test

power_off_cause_test_SOURCES = \
	power_off_cause_test.cpp
power_off_cause_test_CXXFLAGS = \
	$(gtest_cflags)
power_off_cause_test_LDFLAGS = \
	$(OESDK_TESTCASE_FLAGS)
power_off_cause_test_LDADD = \
	$(gtest_ldadd)

power_off_rule_test_SOURCES = \
	power_off_rule_test.cpp \
	../conditions.cpp \
	../json_parser.cpp \
	../logging.cpp
power_off_rule_test_CXXFLAGS = \
	$(gtest_cflags)
power_off_rule_test_LDFLAGS = \
	$(OESDK_TESTCASE_FLAGS)
power_off_rule_test_LDADD = \
	$(gtest_ldadd) \
	$(FMT_LIBS) \
	$(SDBUSPLUS_LIBS) \
	$(SDEVENTPLUS_LIBS)
