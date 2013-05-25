ACLOCAL_AMFLAGS = -I m4

pkgconfdir = $(libdir)/pkgconfig

pkgconf_DATA = libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.pc

SUBDIRS = inc src build docs

EXTRA_DIST = \
	autogen.sh \
	libwps.pc.in \
	libwps-zip.in \
	COPYING.LGPL \
	COPYING.MPL \
	CREDITS \
	HACKING \
	INSTALL \
	NEWS \
	README

distclean-local:
	rm -rf *.cache *~ *.pc

zip: install
	sh libwps-zip

dist-hook:
	git log --date=short --pretty="format:@%cd  %an  <%ae>  [%H]%n%n%s%n%n%e%b" | sed -e "s|^\([^@]\)|\t\1|" -e "s|^@||" >$(distdir)/ChangeLog

