target_test = test

noinst_PROGRAMS = $(target_test)

AM_CXXFLAGS = -I$(top_srcdir)/src/lib/ $(LIBWPS_CXXFLAGS) $(DEBUG_CXXFLAGS)

test_LDFLAGS = -L../lib/ `cppunit-config --libs`
test_LDADD = \
	../lib/libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.la \
	../lib/libwps-stream-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.la \
	$(LIBWPS_LIBS) 

test_SOURCES = \
	test.cpp

.PHONY:		check

check: test
	./test
