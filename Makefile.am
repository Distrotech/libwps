ACLOCAL_AMFLAGS = -I m4

pkgconfdir = $(libdir)/pkgconfig

pkgconf_DATA = libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.pc

SUBDIRS = src build docs autopackage

EXTRA_DIST = \
	autogen.sh \
	libwps.spec \
	libwps.spec.in \
	libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.pc.in \
	libwps-zip.in \
	CHANGES \
	COPYING \
	CREDITS \
	HACKING \
	INSTALL \
	README

distclean-local:
	rm -rf *.cache *~

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

zip: install
	sh libwps-zip

