if OS_WIN32
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -avoid-version
else
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
endif

lib_LTLIBRARIES = libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.la $(target_libwps_stream)
libwps_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_includedir = $(includedir)/libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@/libwps
libwps_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_include_HEADERS = libwps.h WPSDocument.h

INCLUDES = -I$(srcdir)

AM_CXXFLAGS = $(LIBWPS_CFLAGS) $(DEBUG_CXXFLAGS)

libwps_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_la_LIBADD  = $(LIBWPS_LIBS) @LIBWPS_WIN32_RESOURCE@
libwps_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_la_DEPENDENCIES = @LIBWPS_WIN32_RESOURCE@
libwps_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_la_LDFLAGS = $(version_info) -export-dynamic -no-undefined
libwps_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_la_SOURCES = \
	libwps_internal.cpp 	\
	libwps_tools_win.cpp	\
	WPS4.cpp		\
	WPS4Graph.cpp		\
	WPS4Text.cpp		\
	WPS8Text.cpp		\
	WPS8Struct.cpp		\
	WPSCell.cpp		\
	WPSContentListener.cpp	\
	WPSDebug.cpp		\
	WPSDocument.cpp		\
	WPSHeader.cpp		\
	WPSList.cpp		\
	WPSOLEParser.cpp	\
	WPSOLEStream.cpp	\
	WPSPageSpan.cpp		\
	WPSParagraph.cpp	\
	WPSParser.cpp		\
	WPSSubDocument.cpp	\
	WPSTable.cpp		\
	libwps.h		\
	libwps_tools_win.h	\
	libwps_internal.h	\
	WPS.h			\
	WPS4.h			\
	WPS4Graph.h		\
	WPS4Text.h		\
	WPS8Text.h			\
	WPS8Struct.h		\
	WPSCell.h		\
	WPSContentListener.h	\
	WPSDebug.h		\
	WPSDocument.h		\
	WPSEntry.h		\
	WPSHeader.h		\
	WPSList.h		\
	WPSOLEParser.h		\
	WPSOLEStream.h		\
	WPSPageSpan.h		\
	WPSParagraph.h		\
	WPSParser.h		\
	WPSPosition.h		\
	WPSSubDocument.h	\
	WPSTable.h

if OS_WIN32

@LIBWPS_WIN32_RESOURCE@ : libwps.rc $(libwps_@WPS_MAJOR_VERSION@_@WPS_MINOR_VERSION@_la_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource libwps.rc @LIBWPS_WIN32_RESOURCE@

endif

EXTRA_DIST = \
	makefile.mk \
	libwps.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	libwps.rc
