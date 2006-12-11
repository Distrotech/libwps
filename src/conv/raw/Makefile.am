if WITH_LIBWPS_STREAM
target_wps2raw = wps2raw
else
target_wps2raw =
endif

bin_PROGRAMS = $(target_wps2raw)

AM_CXXFLAGS = $(LIBWPS_CXXFLAGS) $(DEBUG_CXXFLAGS) -I$(top_srcdir)/src/lib/

wps2raw_LDFLAGS = -L../../lib/
wps2raw_DEPENDENCIES = @WPS2RAW_WIN32_RESOURCE@
wps2raw_LDADD = \
	-lwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@ \
	-lwps-stream-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@ \
	$(LIBWPS_LIBS) @WPS2RAW_WIN32_RESOURCE@

wps2raw_SOURCES = \
	RawListener.cpp	 \
	RawListener.h	 \
	main.cpp

if OS_WIN32

@WPS2RAW_WIN32_RESOURCE@ : wps2raw.rc
	$(top_srcdir)/build/win32/lt-compile-resource wps2raw.rc @WPS2RAW_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(wps2raw_SOURCES)	\
	wps2raw.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	wps2raw.rc	 
 
