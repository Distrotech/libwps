target_test = test

noinst_PROGRAMS = $(target_test)

AM_CXXFLAGS = $(LIBWPS_CXXFLAGS) $(DEBUG_CXXFLAGS) -I$(top_srcdir)/src/lib/

test_LDFLAGS = -L../lib/ `cppunit-config --libs`
test_LDADD = \
	-lwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@ \
	-lwps-stream-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@ \
	$(LIBWPS_LIBS) 

test_SOURCES = \
	test.cpp

EXTRA_DIST = \
	$(wps2raw_SOURCES)

.PHONY:		check

check: test
	./test
