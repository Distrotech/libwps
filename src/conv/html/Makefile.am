target_wpd2html = wpd2html

bin_PROGRAMS = $(target_wpd2html)

AM_CXXFLAGS = $(LIBWPS_CXXFLAGS) $(DEBUG_CXXFLAGS) -I$(top_srcdir)/src/lib/

wpd2html_LDFLAGS = -L../../lib/
wpd2html_DEPENDENCIES = @WPD2HTML_WIN32_RESOURCE@
wpd2html_LDADD = -lwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@ \
	-lwps-stream-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@ \
	$(LIBWPS_LIBS) @WPD2RAW_WIN32_RESOURCE@

wpd2html_SOURCES = \
	HtmlListenerImpl.cpp	\
	HtmlListenerImpl.h	\
	main.cpp

if OS_WIN32

@WPD2HTML_WIN32_RESOURCE@ : wpd2html.rc
	$(top_srcdir)/build/win32/lt-compile-resource wpd2html.rc @WPD2HTML_WIN32_RESOURCE@
endif

# Include the wpd2html_SOURCES in case we build a tarball without stream
EXTRA_DIST = \
	$(wpd2html_SOURCES)	\
	wpd2html.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	wpd2html.rc	 

