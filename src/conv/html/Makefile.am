if WITH_LIBWPS_STREAM
target_wps2html = wps2html
else
target_wps2html =
endif

bin_PROGRAMS = $(target_wps2html)

AM_CXXFLAGS = $(LIBWPS_CXXFLAGS) $(DEBUG_CXXFLAGS) -I$(top_srcdir)/src/lib/

wps2html_LDFLAGS = -L../../lib/
wps2html_DEPENDENCIES = @WPS2HTML_WIN32_RESOURCE@
wps2html_LDADD = \
	-lwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@ \
	-lwps-stream-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@ \
	$(LIBWPS_LIBS) @WPS2HTML_WIN32_RESOURCE@

wps2html_SOURCES = \
	HtmlListenerImpl.cpp	\
	HtmlListenerImpl.h	\
	wps2html.cpp

if OS_WIN32

@WPS2HTML_WIN32_RESOURCE@ : wps2html.rc
	$(top_srcdir)/build/win32/lt-compile-resource wps2html.rc @WPS2HTML_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	wps2html.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	wps2html.rc	 

