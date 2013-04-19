bin_PROGRAMS = wps2raw

AM_CXXFLAGS = -I$(top_srcdir)/src/lib/ $(WPD_CFLAGS) $(DEBUG_CXXFLAGS)

wps2raw_DEPENDENCIES = @WPS2RAW_WIN32_RESOURCE@

if STATIC_TOOLS

wps2raw_LDADD = \
	../../lib/@WPS_OBJDIR@/libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.a \
	$(WPD_LIBS) @WPS2RAW_WIN32_RESOURCE@
wps2raw_LDFLAGS = -all-static

else	

wps2raw_LDADD = \
	../../lib/libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.la \
	$(WPD_LIBS) @WPS2RAW_WIN32_RESOURCE@

endif

wps2raw_SOURCES = \
	RawDocumentGenerator.cpp	 \
	RawDocumentGenerator.h	 \
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
 
