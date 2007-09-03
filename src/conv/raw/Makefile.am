if WITH_LIBWPS_STREAM
target_wps2raw = wps2raw
else
target_wps2raw =
endif

bin_PROGRAMS = $(target_wps2raw)

AM_CXXFLAGS = -I$(top_srcdir)/src/lib/ $(LIBWPS_CXXFLAGS) $(DEBUG_CXXFLAGS)

wps2raw_DEPENDENCIES = @WPS2RAW_WIN32_RESOURCE@

if STATIC_TOOLS

wps2raw_LDADD = \
	../../lib/@WPS_OBJDIR@/libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.a \
	../../lib/@WPS_OBJDIR@/libwps-stream-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.a \
	$(LIBWPS_LIBS) @WPS2RAW_WIN32_RESOURCE@
wps2raw_LDFLAGS = -all-static

else	

wps2raw_LDADD = \
	../../lib/libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.la \
	../../lib/libwps-stream-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.la \
	$(LIBWPS_LIBS) @WPS2RAW_WIN32_RESOURCE@

endif

wps2raw_SOURCES = \
	RawListener.cpp	 \
	RawListener.h	 \
	wps2raw.cpp

if OS_WIN32

@WPS2RAW_WIN32_RESOURCE@ : wps2raw.rc $(wps2raw_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource wps2raw.rc @WPS2RAW_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(wps2raw_SOURCES)	\
	wps2raw.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	wps2raw.rc	 
 
