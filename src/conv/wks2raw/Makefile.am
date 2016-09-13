if BUILD_TOOLS

bin_PROGRAMS = wks2raw

AM_CXXFLAGS = -I$(top_srcdir)/inc -I../helper $(REVENGE_CFLAGS) $(REVENGE_GENERATORS_CFLAGS) $(REVENGE_STREAM_CFLAGS) $(DEBUG_CXXFLAGS)

wks2raw_DEPENDENCIES = @WKS2RAW_WIN32_RESOURCE@

if STATIC_TOOLS

wks2raw_LDADD = \
	../../lib/@WPS_OBJDIR@/libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.a \
	../helper/@WPS_OBJDIR@/libconvHelper.a $(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS) @WKS2RAW_WIN32_RESOURCE@
wks2raw_LDFLAGS = -all-static

else	

wks2raw_LDADD = \
	../../lib/libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.la \
	../helper/libconvHelper.la $(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS)  @WKS2RAW_WIN32_RESOURCE@

endif

wks2raw_SOURCES = \
	wks2raw.cpp

if OS_WIN32

@WKS2RAW_WIN32_RESOURCE@ : wks2raw.rc $(wks2raw_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource wks2raw.rc @WKS2RAW_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(wks2raw_SOURCES)	\
	wks2raw.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	wks2raw.rc	 

endif
