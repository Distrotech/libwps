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
	$(INSTALL) .libs/libwps*-0.1.dll.a $(DESTDIR)$(libdir)

uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libwps*-0.1.dll.a

else
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
install-libtool-import-lib:
uninstall-libtool-import-lib:
endif

lib_LTLIBRARIES = libwps-0.1.la
libwps_0_1_includedir = $(includedir)/libwps-0.1/libwps
libwps_0_1_include_HEADERS = libwps.h libwpd_types.h WPXStream.h  \
WPSDocument.h GSFStream.h

INCLUDES = -I$(srcdir)

AM_CXXFLAGS = $(LIBWPS_CXXFLAGS) $(DEBUG_CXXFLAGS)

libwps_0_1_la_LIBADD  = @LIBWPS_WIN32_RESOURCE@
libwps_0_1_la_DEPENDENCIES = @LIBWPS_WIN32_RESOURCE@
libwps_0_1_la_LDFLAGS = $(version_info) -export-dynamic $(no_undefined)
libwps_0_1_la_SOURCES = \
	libwpd_internal.cpp \
	libwpd_math.cpp \
	WPS4.cpp \
	WPS8.cpp \
	WPSDocument.cpp \
	WPXContentListener.cpp \
	WPXHeader.cpp \
	WPXListener.cpp \
	WPXMemoryStream.cpp \
	WPXPageSpan.cpp \
	WPXParser.cpp \
	WPXPart.cpp \
	WPXStylesListener.cpp \
	WPXSubDocument.cpp \
	WPXTable.cpp \
	libwps.h \
	libwpd_internal.h \
	libwpd_math.h \
	libwpd_types.h \
	WPSDocument.h \
	WPXContentListener.h \
	WPXHeader.h \
	WPXFileStructure.h \
	WPXListener.h \
	WPXMemoryStream.h \
	WPXPageSpan.h \
	WPXParser.h \
	WPXPart.h \
	WPXStylesListener.h \
	WPXSubDocument.h \
	WPXTable.h \
	GSFStream.cpp

if OS_WIN32

@LIBWPS_WIN32_RESOURCE@ : libwpd.rc
	$(top_srcdir)/build/win32/lt-compile-resource libwpd.rc @LIBWPS_WIN32_RESOURCE@

endif


install-data-local: install-libtool-import-lib

uninstall-local: uninstall-libtool-import-lib

EXTRA_DIST = \
	makefile.mk \
	libwpd.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	libwpd.rc 
