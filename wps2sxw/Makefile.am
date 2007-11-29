SUBDIRS = build filter cmdline

EXTRA_DIST =	\
	wps2sxw.spec.in			\
	wps2sxw.spec			\
	autogen.sh			\
	CHANGES README

distclean-local:
	rm -rf *.cache *~ *.out

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

zip: install
	sh wps2sxw-zip
