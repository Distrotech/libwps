AC_INIT(src/lib/WPS8.cpp)
AC_PREREQ([2.65])

AC_CONFIG_MACRO_DIR([m4])
AM_CONFIG_HEADER([config.h])

WPS_MAJOR_VERSION=0
WPS_MINOR_VERSION=2
WPS_MICRO_VERSION=2

WPS_VERSION="$WPS_MAJOR_VERSION.$WPS_MINOR_VERSION.$WPS_MICRO_VERSION"

AC_SUBST(WPS_MAJOR_VERSION)
AC_SUBST(WPS_MINOR_VERSION)
AC_SUBST(WPS_MICRO_VERSION)
AC_SUBST(WPS_VERSION)

dnl # libtool versioning
dnl LT_RELEASE=$WPS_MAJOR_VERSION.$WPS_MINOR_VERSION
dnl AC_SUBST(LT_RELEASE)

LT_CURRENT=`expr 10 '*' $WPS_MAJOR_VERSION + $WPS_MINOR_VERSION`
LT_REVISION=$WPS_MICRO_VERSION
# For 1.0.0 comment the first line and uncomment the second
LT_AGE=0
# LT_AGE=$WPS_MINOR_VERSION
LT_CURRENT_MINUS_AGE=`expr $LT_CURRENT - $LT_AGE`
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)
AC_SUBST(LT_CURRENT_MINUS_AGE)

AM_INIT_AUTOMAKE(libwps, $WPS_VERSION, foreign dist-bzip2)
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

AC_ARG_ENABLE([static-tools],
	[AS_HELP_STRING([--enable-static-tools], [Link tools (binaries) statically])]
)
AS_IF([test "x$enable_static_tools" = "xyes"], [
	enable_static="yes"
], [
	AC_DISABLE_STATIC
])
AM_CONDITIONAL(STATIC_TOOLS, [test "x$enable_static_tools" = "xyes"])

AC_ARG_ENABLE([werror],
	[AS_HELP_STRING([--disable-werror], [Treat all warnings as errors, usefull for development])]
)
AS_IF([test x"$enable_werror" != "xno"], [
	CFLAGS="$CFLAGS -Werror"
	CXXFLAGS="$CXXFLAGS -Werror"
])

AC_SUBST(WPS_OBJDIR, [$objdir])

AC_PATH_PROG(PKG_CONFIG, [pkg-config], [no])

dnl ***********************************************************************************

PKG_CHECK_MODULES(LIBWPS,[
    libwpd-0.9
	libwpd-stream-0.9
    ])

AC_MSG_CHECKING([for native Win32])
case "$host" in
	*-*-mingw*)
		native_win32=yes
		LIBWPS_WIN32_RESOURCE=libwps-win32res.lo
		LIBWPS_STREAM_WIN32_RESOURCE=libwps-stream-win32res.lo
		WPS2HTML_WIN32_RESOURCE=wps2html-win32res.lo
		WPS2TEXT_WIN32_RESOURCE=wps2text-win32res.lo
		WPS2RAW_WIN32_RESOURCE=wps2raw-win32res.lo
	;;
	*)
		native_win32=no
		LIBWPS_WIN32_RESOURCE=
		LIBWPS_STREAM_WIN32_RESOURCE=
		WPS2HTML_WIN32_RESOURCE=
		WPS2TEXT_WIN32_RESOURCE=
		WPS2RAW_WIN32_RESOURCE=
	;;
esac
AC_MSG_RESULT([$native_win32])
AM_CONDITIONAL(OS_WIN32, [test "x$native_win32" = "xyes"])
AC_SUBST(LIBWPS_WIN32_RESOURCE)
AC_SUBST(LIBWPS_STREAM_WIN32_RESOURCE)
AC_SUBST(WPS2HTML_WIN32_RESOURCE)
AC_SUBST(WPS2TEXT_WIN32_RESOURCE)
AC_SUBST(WPS2RAW_WIN32_RESOURCE)

AC_ARG_WITH(docs,
	[AS_HELP_STRING([--without-docs], [Do not build documentation])]
)
AS_IF([test "x$native_win32" = "xyes"], [with_docs=false])
AS_IF([test "x$with_docs" != "xno"], [
	AC_PATH_PROG(DOXYGEN, [doxygen])
	AS_IF([test -z "$DOXYGEN"], [
		AC_MSG_WARN([*** Could not find doxygen in your PATH.])
		AC_MSG_WARN([*** The documentation will not be built.])
		enable_docs=false
	], [enable_docs=true])
], [enable_docs=false])
AM_CONDITIONAL([WITH_LIBWPS_DOCS], [test "x$enable_docs" != "xfalse"])

AC_MSG_CHECKING([for Win32 platform in general])
case "$host" in
	*-*-mingw*|*-*-cygwin*)
		platform_win32=yes
	;;
	*)
		platform_win32=no
	;;
esac
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL(PLATFORM_WIN32, [test "x$platform_win32" = "xyes"])

AC_MSG_CHECKING([for Darwin (Mac OS X) platform])
case "$host" in
	*-*-darwin*)
		platform_darwin=yes
	;;
	*)
		platform_darwin=no
	;;
esac
AC_MSG_RESULT([$platform_darwin])
AM_CONDITIONAL(PLATFORM_DARWIN, [test "x$platform_darwin" = "xyes"])

AC_ARG_ENABLE([debug],
	[AS_HELP_STRING([--enable-debug], [Turn on debugging])]
)
AS_IF([test "x$enable_debug" = "xyes"], [
	DEBUG_CXXFLAGS="-DDEBUG -g"
], [
	DEBUG_CXXFLAGS="-DNDEBUG"
])

# Courtesy of Glib: Ensure MSVC-compatible struct packing convention
# is used when compiling for Win32 with gcc.
AS_IF([test "x$native_win32" = "xyes"], [
	AC_CHECK_TOOL(WINDRES, windres)
	AS_IF([x"$GCC" = xyes], [
		AC_MSG_CHECKING([how to get MSVC-compatible struct packing])
		AS_IF([test -z "$ac_cv_prog_CC"], [
			our_gcc="$CC"
		], [
			our_gcc="$ac_cv_prog_CC"
		])
		AS_IF([$our_gcc -v --help 2>/dev/null | grep ms-bitfields >/dev/null], [
			msnative_struct="-mms-bitfields"
			CFLAGS="$CFLAGS $msnative_struct"
			CXXFLAGS="$CXXFLAGS $msnative_struct"
			AC_MSG_RESULT([${msnative_struct}])
		], [
			AC_MSG_RESULT([no way])
			AC_MSG_WARN([produced libraries might be incompatible with MSVC-compiled code])
		])
	])
	CFLAGS="$CFLAGS -Wall -pedantic"
	CXXFLAGS="$CXXFLAGS -Wall -pedantic"
], [
	AS_IF([test x"$GCC" = xyes], [
		# Be tough with warnings and produce less careless code
		CFLAGS="$CFLAGS -Wall -pedantic"
		CXXFLAGS="$CXXFLAGS -Wall -pedantic"
	])
])

LIBWPS_CXXFLAGS="$LIBWPS_CFLAGS"
AC_SUBST(LIBWPS_CXXFLAGS)
AC_SUBST(DEBUG_CXXFLAGS)
AC_SUBST(LIBWPS_LIBS)

AC_OUTPUT([
Makefile
src/Makefile
src/conv/Makefile
src/conv/html/Makefile
src/conv/html/wps2html.rc
src/conv/raw/Makefile
src/conv/raw/wps2raw.rc
src/conv/text/Makefile
src/conv/text/wps2text.rc
src/lib/Makefile
src/lib/libwps.rc
docs/Makefile
docs/doxygen/Makefile
build/Makefile
build/win32/Makefile
libwps-0.2.pc
libwps.spec
libwps-zip
autopackage/Makefile
autopackage/default.apspec
])

AS_IF([test "x$enable_debug" = "xyes"], [
	AC_MSG_RESULT([Configured with debugging symbols.])
])
