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
	$(INSTALL) .libs/libwpd*-0.8.dll.a $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libwpd*-0.8.dll.a

else
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

if WITH_LIBWPD_STREAM
target_libwpd_stream = libwpd-stream-0.8.la
else
target_libwpd_stream =
endif

lib_LTLIBRARIES = libwpd-0.8.la $(target_libwpd_stream)
libwpd_0_8_includedir = $(includedir)/libwpd-0.8/libwpd
libwpd_0_8_include_HEADERS = libwpd.h libwpd_types.h WPXStream.h WPXProperty.h WPXPropertyList.h \
WPXString.h WPXPropertyListVector.h WPDocument.h WPXHLListenerImpl.h

INCLUDES = -I$(srcdir)

AM_CXXFLAGS = $(LIBWPD_CXXFLAGS) $(DEBUG_CXXFLAGS)

libwpd_0_8_la_LIBADD  = @LIBWPD_WIN32_RESOURCE@
libwpd_0_8_la_DEPENDENCIES = @LIBWPD_WIN32_RESOURCE@
libwpd_0_8_la_LDFLAGS = $(version_info) -export-dynamic $(no_undefined)
libwpd_0_8_la_SOURCES = \
	libwpd_internal.cpp \
	libwpd_math.cpp \
	WPS4.cpp \
	WPS8.cpp \
	WPDocument.cpp \
	WPXContentListener.cpp \
	WPXHeader.cpp \
	WPXListener.cpp \
	WPXMemoryStream.cpp \
	WPXPageSpan.cpp \
	WPXParser.cpp \
	WPXPart.cpp \
	WPXProperty.cpp \
	WPXPropertyList.cpp \
	WPXPropertyListVector.cpp \
	WPXString.cpp \
	WPXStylesListener.cpp \
	WPXSubDocument.cpp \
	WPXTable.cpp \
	libwpd.h \
	libwpd_internal.h \
	libwpd_math.h \
	libwpd_types.h \
	WPDocument.h \
	WPXContentListener.h \
	WPXHeader.h \
	WPXFileStructure.h \
	WPXHLListenerImpl.h \
	WPXListener.h \
	WPXMemoryStream.h \
	WPXPageSpan.h \
	WPXParser.h \
	WPXPart.h \
	WPXProperty.h \
	WPXPropertyList.h \
	WPXPropertyListVector.h \
	WPXString.h \
	WPXStylesListener.h \
	WPXSubDocument.h \
	WPXTable.h

libwpd_stream_headers = GSFStream.h
libwpd_stream_sources = GSFStream.cpp

if WITH_LIBWPD_STREAM

libwpd_stream_0_8_includedir = $(includedir)/libwpd-0.8/libwpd
libwpd_stream_0_8_include_HEADERS = $(libwpd_stream_headers)

libwpd_stream_0_8_la_LIBADD  = $(LIBWPD_LIBS) @LIBWPD_STREAM_WIN32_RESOURCE@
libwpd_stream_0_8_la_DEPENDENCIES = @LIBWPD_STREAM_WIN32_RESOURCE@
libwpd_stream_0_8_la_LDFLAGS = $(version_info) -export-dynamic $(no_undefined)
libwpd_stream_0_8_la_SOURCES = $(libwpd_stream_sources)

endif

if OS_WIN32

@LIBWPD_WIN32_RESOURCE@ : libwpd.rc
	$(top_srcdir)/build/win32/lt-compile-resource libwpd.rc @LIBWPD_WIN32_RESOURCE@

@LIBWPD_STREAM_WIN32_RESOURCE@ : libwpd-stream.rc
	$(top_srcdir)/build/win32/lt-compile-resource libwpd-stream.rc @LIBWPD_STREAM_WIN32_RESOURCE@
endif


install-data-local: install-libtool-import-lib

uninstall-local: uninstall-libtool-import-lib

# Include the GSFStream.* in case we build a tarball without stream
EXTRA_DIST = \
	$(libwpd_stream_sources) \
	$(libwpd_stream_headers) \
	makefile.mk \
	libwpd.rc.in \
	libwpd-stream.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	libwpd.rc \
	libwpd-stream.rc
