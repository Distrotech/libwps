ACLOCAL_AMFLAGS = -I m4

pkgconfdir = $(libdir)/pkgconfig

pkgconf_DATA = libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.pc

SUBDIRS = src build docs

EXTRA_DIST = \
	autogen.sh \
	libwps.spec \
	libwps.spec.in \
	libwps-@WPS_MAJOR_VERSION@.@WPS_MINOR_VERSION@.pc.in \
	libwps-zip.in \
	COPYING \
	CREDITS \
	HACKING \
	INSTALL \
	NEWS \
	README

distclean-local:
	rm -rf *.cache *~

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

zip: install
	sh libwps-zip

dist-hook:
	git log --date=short --pretty="format:@%cd  %an  <%ae>  [%H]%n%n%s%n%n%e%b" | sed -e "s|^\([^@]\)|\t\1|" -e "s|^@||" >$(distdir)/ChangeLog

