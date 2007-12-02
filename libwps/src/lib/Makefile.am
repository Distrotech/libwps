if PLATFORM_WIN32
no_undefined = -no-undefined
else
if PLATFORM_DARWIN
no_undefined = -no-undefined
else
no_undefined =
endif
endif

if OS_WIN32
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -avoid-version

install-libtool-import-lib:
	$(INSTALL) @WPS_OBJDIR@/libwps*-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.dll.a $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libwps*-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.dll.a

else
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if WITH_LIBWPS_STREAM
target_libwps_stream = libwps-stream-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.la
else
target_libwps_stream =
endif


lib_LTLIBRARIES = libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.la $(target_libwps_stream)
libwps_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_includedir = $(includedir)/libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@/libwps
libwps_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_include_HEADERS = libwps.h WPSDocument.h WPSStream.h

INCLUDES = -I$(srcdir)

AM_CXXFLAGS = $(LIBWPS_CXXFLAGS) $(DEBUG_CXXFLAGS)

libwps_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_la_LIBADD  = $(LIBWPS_LIBS) @LIBWPS_WIN32_RESOURCE@
libwps_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_la_DEPENDENCIES = @LIBWPS_WIN32_RESOURCE@
libwps_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_la_LDFLAGS = $(version_info) -export-dynamic $(no_undefined)
libwps_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_la_SOURCES = \
	libwps_internal.cpp \
	WPS4.cpp \
	WPS8.cpp \
	WPSDocument.cpp \
	WPSContentListener.cpp \
	WPSHeader.cpp \
	WPSPageSpan.cpp \
	WPSParser.cpp \
	libwps.h \
	libwps_internal.h \
	WPS.h \
	WPS4.h \
	WPS8.h \
	WPSDocument.h \
	WPSContentListener.h \
	WPSHeader.h \
	WPSFileStructure.h \
	WPSPageSpan.h \
	WPSParser.h

libwps_stream_headers = WPSStreamImplementation.h
libwps_stream_sources = \
	WPSStreamImplementation.cpp \
	WPSOLEStream.cpp \
	WPSOLEStream.h

if WITH_LIBWPS_STREAM

libwps_stream_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_includedir = $(includedir)/libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@/libwps
libwps_stream_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_include_HEADERS = $(libwps_stream_headers)

libwps_stream_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_la_LIBADD  = @LIBWPS_STREAM_WIN32_RESOURCE@
libwps_stream_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_la_DEPENDENCIES = @LIBWPS_STREAM_WIN32_RESOURCE@
libwps_stream_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_la_LDFLAGS = $(version_info) -export-dynamic $(no_undefined)
libwps_stream_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_la_SOURCES = $(libwps_stream_sources)

endif



if OS_WIN32

@LIBWPS_WIN32_RESOURCE@ : libwps.rc $(libwps_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_la_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource libwps.rc @LIBWPS_WIN32_RESOURCE@

if WITH_LIBWPS_STREAM

@LIBWPS_STREAM_WIN32_RESOURCE@ : libwps-stream.rc $(libwps_stream_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_la_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource libwps-stream.rc @LIBWPS_STREAM_WIN32_RESOURCE@

endif

endif


install-data-local: install-libtool-import-lib

uninstall-local: uninstall-libtool-import-lib

EXTRA_DIST = \
	$(libwps_stream_sources) \
	$(libwps_stream_headers) \
	makefile.mk \
	libwps.rc.in \
	libwps-stream.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	libwps.rc \
	libwps-stream.rc
