all:	vkb_compiler ukeyboard-prefs keyboards keyboards-scv

ukeyboard-prefs:
	$(MAKE) -C cpanel

vkb_compiler:	vkb_compiler.c
	gcc -W -Wall -o vkb_compiler vkb_compiler.c

keyboards:	vkb_compiler
	$(MAKE) -C keyboards

keyboards-scv:	vkb_compiler
	$(MAKE) -C keyboards-scv

install:	ukeyboard-prefs keyboards-scv
	$(MAKE) -C cpanel DESTDIR=$(DESTDIR) install
	$(MAKE) -C keyboards DESTDIR=$(DESTDIR) install
	$(MAKE) -C keyboards-scv DESTDIR=$(DESTDIR) install
#	install -d $(DESTDIR)/usr/share/icons/hicolor/26x26/apps
#	install -m 0644 ukeyboard.png $(DESTDIR)/usr/share/icons/hicolor/26x26/apps/
	install -d $(DESTDIR)/usr/share/X11/xkb/symbols/nokia_vndr
	install -m 0644 xkb/ukeyboard $(DESTDIR)/usr/share/X11/xkb/symbols/nokia_vndr/

clean:
	$(MAKE) -C cpanel clean
	$(MAKE) -C keyboards clean
	$(MAKE) -C keyboards-scv clean
	rm -f vkb_compiler
	rm -rf build

distclean:
	$(MAKE) clean
	rm -f *-stamp
	rm -rf debian/ukeyboard debian/files
