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

if WITH_LIBWPS_DOCS
install-data-local:
	mkdir -p $(DESTDIR)$(docdir)
	$(INSTALL_DATA) $(srcdir)/CHANGES $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) $(srcdir)/COPYING $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) $(srcdir)/CREDITS $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) $(srcdir)/HACKING $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) $(srcdir)/INSTALL $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) $(srcdir)/README $(DESTDIR)$(docdir)/
uninstall-local:
	-rm -rf $(DESTDIR)$(docdir)
endif
