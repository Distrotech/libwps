bin_PROGRAMS = wps2sxw

AM_CXXFLAGS = $(WPS2SXW_CXXFLAGS) $(DEBUG_CXXFLAGS) -I$(top_srcdir)/filter 

wps2sxw_DEPENDENCIES = @WPS2SXW_WIN32_RESOURCE@ ../filter/libwps2sxw-1.la
wps2sxw_LDADD = $(WPS2SXW_LIBS) @WPS2SXW_WIN32_RESOURCE@ ../filter/libwps2sxw-1.la

if STATIC_TOOLS
wps2sxw_LDFLAGS = -all-static
endif

wps2sxw_SOURCES = \
	DiskDocumentHandler.hxx \
	DiskDocumentHandler.cxx \
	femtozip.cxx \
	femtozip.hxx \
	StdOutHandler.hxx \
	StdOutHandler.cxx \
	wps2sxw.cxx

if OS_WIN32

@WPS2SXW_WIN32_RESOURCE@ : wps2sxw.rc $(wps2sxw_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource wps2sxw.rc @WPS2SXW_WIN32_RESOURCE@
endif 
