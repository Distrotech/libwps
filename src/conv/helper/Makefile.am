if BUILD_TOOLS
AM_CXXFLAGS = -I$(top_srcdir)/inc/ $(REVENGE_CFLAGS) $(REVENGE_GENERATORS_CFLAGS) $(REVENGE_STREAM_CFLAGS) $(DEBUG_CXXFLAGS)

noinst_LTLIBRARIES = libconvHelper.la

libconvHelper_la_LIBADD =
libconvHelper_la_CPPFLAGS = $(XATTR_CFLAGS)
libconvHelper_la_SOURCES = helper.h helper.cpp
endif
