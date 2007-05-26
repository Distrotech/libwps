if WITH_LIBWPS_STREAM
target_wps2text = wps2text
else
target_wps2text =
endif

bin_PROGRAMS = $(target_wps2text)

AM_CXXFLAGS = $(LIBWPS_CXXFLAGS) $(DEBUG_CXXFLAGS) -I$(top_srcdir)/src/lib/

wps2text_LDFLAGS = -L../../lib/
wps2text_DEPENDENCIES = @WPS2TEXT_WIN32_RESOURCE@
wps2text_LDADD = \
	-lwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@ \
	-lwps-stream-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@ \
	$(LIBWPS_LIBS) @WPS2TEXT_WIN32_RESOURCE@

wps2text_SOURCES = \
	TextListenerImpl.cpp	 \
	TextListenerImpl.h	 \
	wps2text.cpp

if OS_WIN32

@WPS2TEXT_WIN32_RESOURCE@ : wps2text.rc
	$(top_srcdir)/build/win32/lt-compile-resource wps2text.rc @WPS2TEXT_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(wps2text_SOURCES)	\
	wps2text.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	wps2text.rc	 
 
