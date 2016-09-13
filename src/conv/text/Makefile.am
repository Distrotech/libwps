if BUILD_TOOLS

bin_PROGRAMS = wps2text

AM_CXXFLAGS = -I$(top_srcdir)/inc -I../helper $(REVENGE_CFLAGS) $(REVENGE_GENERATORS_CFLAGS) $(REVENGE_STREAM_CFLAGS) $(DEBUG_CXXFLAGS)

wps2text_DEPENDENCIES = @WPS2TEXT_WIN32_RESOURCE@

if STATIC_TOOLS

wps2text_LDADD = \
	../../lib/@WPS_OBJDIR@/libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.a \
	../helper/@WPS_OBJDIR@/libconvHelper.a $(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS) @WPS2TEXT_WIN32_RESOURCE@
wps2text_LDFLAGS = -all-static

else	

wps2text_LDADD = \
	../../lib/libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.la \
	../helper/libconvHelper.la $(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS) @WPS2TEXT_WIN32_RESOURCE@

endif

wps2text_SOURCES = \
	wps2text.cpp

if OS_WIN32

@WPS2TEXT_WIN32_RESOURCE@ : wps2text.rc $(wps2text_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource wps2text.rc @WPS2TEXT_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(wps2text_SOURCES)	\
	wps2text.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	wps2text.rc	 

endif
