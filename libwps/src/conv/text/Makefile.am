if WITH_LIBWPS_STREAM
target_wps2text = wps2text
else
target_wps2text =
endif

bin_PROGRAMS = $(target_wps2text)

AM_CXXFLAGS = -I$(top_srcdir)/src/lib/ $(LIBWPS_CXXFLAGS) $(DEBUG_CXXFLAGS)

wps2text_DEPENDENCIES = @WPS2TEXT_WIN32_RESOURCE@

if STATIC_TOOLS

wps2text_LDADD = \
	../../lib/@WPS_OBJDIR@/libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.a \
	../../lib/@WPS_OBJDIR@/libwps-stream-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.a \
	$(LIBWPS_LIBS) @WPS2TEXT_WIN32_RESOURCE@
wps2text_LDFLAGS = -all-static

else	

wps2text_LDADD = \
	../../lib/libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.la \
	../../lib/libwps-stream-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.la \
	$(LIBWPS_LIBS) @WPS2TEXT_WIN32_RESOURCE@

endif

wps2text_SOURCES = \
	TextListenerImpl.cpp	 \
	TextListenerImpl.h	 \
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
 
