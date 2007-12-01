#!/bin/sh

TESTAUTOHEADER="autoheader autoheader-2.53"
TESTAUTOCONF="autoconf autoconf-2.53"
TESTLIBTOOLIZE="glibtoolize libtoolize"

AUTOHEADERFOUND="0"
AUTOCONFFOUND="0"
LIBTOOLIZEFOUND="0"

##
## Look for autoheader
##
for i in $TESTAUTOHEADER; do
        if which $i > /dev/null 2>&1; then
                if [ `$i --version | head -n 1 | cut -d.  -f 2` -ge 53 ]; then
                        AUTOHEADER=$i
                        AUTOHEADERFOUND="1"
                        break
                fi
        fi
done

##
## Look for autoconf
##

for i in $TESTAUTOCONF; do
        if which $i > /dev/null 2>&1; then
                if [ `$i --version | head -n 1 | cut -d.  -f 2` -ge 53 ]; then
                        AUTOCONF=$i
                        AUTOCONFFOUND="1"
                        break
                fi
        fi
done

for i in $TESTLIBTOOLIZE; do
	if which $i > /dev/null 2>&1; then
		LIBTOOLIZE=$i
		LIBTOOLIZEFOUND="1"
		break
	fi
done

##
## do we have it?
##
if [ "$AUTOCONFFOUND" = "0" -o "$AUTOHEADERFOUND" = "0" ]; then
        echo "$0: need autoconf 2.53 or later to build wps2sxw" >&2
        exit 1
fi

if [ "$LIBTOOLIZEFOUND" = "0" ]; then
	echo "$0: need libtoolize tool to build wps2sxw" >&2
	exit 1
fi

rm -rf autom4te*.cache

aclocal --version > /dev/null 2> /dev/null || {
    echo "error: aclocal not found"
    exit 1
}
automake --version > /dev/null 2> /dev/null || {
    echo "error: automake not found"
    exit 1
}

amcheck=`automake --version | grep 'automake (GNU automake) 1.5'`
if test "x$amcheck" = "xautomake (GNU automake) 1.5"; then
    echo "warning: you appear to be using automake 1.5"
    echo "         this version has a bug - GNUmakefile.am dependencies are not generated"
fi

$LIBTOOLIZE --force --copy || {
    echo "error: libtoolize failed"
    exit 1
}
aclocal $ACLOCAL_FLAGS || {
    echo "error: aclocal $ACLOCAL_FLAGS failed"
    exit 1
}
$AUTOHEADER || {
    echo "error: autoheader failed"
    exit 1
}
automake -a -c --foreign || {
    echo "warning: automake failed"
}
$AUTOCONF || {
    echo "error: autoconf failed"
    exit 1
}
