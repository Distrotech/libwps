if BUILD_TOOLS

bin_PROGRAMS = wks2text

AM_CXXFLAGS = -I$(top_srcdir)/inc $(REVENGE_CFLAGS) $(REVENGE_GENERATORS_CFLAGS) $(REVENGE_STREAM_CFLAGS) $(DEBUG_CXXFLAGS)

wks2text_DEPENDENCIES = @WKS2TEXT_WIN32_RESOURCE@

if STATIC_TOOLS

wks2text_LDADD = \
	../../lib/@WPS_OBJDIR@/libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.a \
	$(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS) @WKS2TEXT_WIN32_RESOURCE@
wks2text_LDFLAGS = -all-static

else	

wks2text_LDADD = \
	../../lib/libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.la \
	$(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS)  @WKS2TEXT_WIN32_RESOURCE@

endif

wks2text_SOURCES = \
	wks2text.cpp

if OS_WIN32

@WKS2TEXT_WIN32_RESOURCE@ : wks2text.rc $(wks2raw_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource wks2text.rc @WKS2TEXT_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(wks2text_SOURCES)	\
	wks2text.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	wks2text.rc	 

endif
