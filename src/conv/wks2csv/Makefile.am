if BUILD_TOOLS

bin_PROGRAMS = wks2csv

AM_CXXFLAGS = -I$(top_srcdir)/inc -I../helper $(REVENGE_CFLAGS) $(REVENGE_GENERATORS_CFLAGS) $(REVENGE_STREAM_CFLAGS) $(DEBUG_CXXFLAGS)

wks2csv_DEPENDENCIES = @WKS2CSV_WIN32_RESOURCE@

if STATIC_TOOLS

wks2csv_LDADD = \
	../../lib/@WPS_OBJDIR@/libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.a \
	../helper/@WPS_OBJDIR@/libconvHelper.a $(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS) @WKS2CSV_WIN32_RESOURCE@
wks2csv_LDFLAGS = -all-static

else	

wks2csv_LDADD = \
	../../lib/libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.la \
	../helper/libconvHelper.la $(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS)  @WKS2CSV_WIN32_RESOURCE@

endif

wks2csv_SOURCES = \
	wks2csv.cpp

if OS_WIN32

@WKS2CSV_WIN32_RESOURCE@ : wks2csv.rc $(wks2csv_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource wks2csv.rc @WKS2CSV_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(wks2csv_SOURCES)	\
	wks2csv.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	wks2csv.rc	 

endif
