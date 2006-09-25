SUBDIRS = doxygen

EXTRA_DIST = libwpd.dia libwpd.png

if WITH_LIBWPS_DOCS

install-data-am:
	mkdir -p $(DESTDIR)$(datadir)/doc/libwps-@WPS_VERSION@
	$(INSTALL) $(srcdir)/libwpd.dia $(DESTDIR)$(datadir)/doc/libwps-@WPS_VERSION@/
	$(INSTALL) $(srcdir)/libwpd.png $(DESTDIR)$(datadir)/doc/libwps-@WPS_VERSION@/

uninstall-am:
	-rm -f $(DESTDIR)$(datadir)/doc/libwps-@WPS_VERSION@/libwpd.dia
	-rm -f $(DESTDIR)$(datadir)/doc/libwps-@WPS_VERSION@/libwpd.png

endif
