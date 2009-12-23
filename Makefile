all:	vkb_compiler ukeyboard-prefs keyboards keyboards-scv ukbdcreator

ukeyboard-prefs:
	$(MAKE) -C cpanel

vkb_compiler:	vkb_compiler.o vkb_compiler_lib.o
	$(CC) -o $@ $+

vkb_compiler.o:	vkb_compiler.c vkb_compiler.h
	$(CC) -W -Wall -c -o $@ $<

vkb_compiler_lib.o:	vkb_compiler_lib.c vkb_compiler.h
	$(CC) -W -Wall -c -o $@ $<

keyboards:	vkb_compiler
	$(MAKE) -C keyboards

keyboards-scv:	vkb_compiler
	$(MAKE) -C keyboards-scv

ukbdcreator:	vkb_compiler_lib.o
	$(MAKE) -C ukbdcreator

install:	ukeyboard-prefs keyboards-scv
	$(MAKE) -C cpanel DESTDIR=$(DESTDIR) install
	$(MAKE) -C keyboards DESTDIR=$(DESTDIR) install
	$(MAKE) -C keyboards-scv DESTDIR=$(DESTDIR) install
	install -d $(DESTDIR)/usr/share/icons/hicolor/48x48/apps
	install -m 0644 ukeyboard.png $(DESTDIR)/usr/share/icons/hicolor/48x48/apps/
	install -d $(DESTDIR)/usr/share/X11/xkb/symbols/nokia_vndr
	install -m 0644 xkb/symbols/ukeyboard $(DESTDIR)/usr/share/X11/xkb/symbols/nokia_vndr/
	install -d $(DESTDIR)/usr/share/X11/xkb/types
	install -m 0644 xkb/types/ukeyboard $(DESTDIR)/usr/share/X11/xkb/types/
	$(MAKE) -C ukbdcreator DESTDIR=$(DESTDIR) install

clean:
	$(MAKE) -C cpanel clean
	$(MAKE) -C keyboards clean
	$(MAKE) -C keyboards-scv clean
	$(MAKE) -C ukbdcreator clean
	rm -f vkb_compiler *.o core
	rm -rf build

distclean:
	$(MAKE) clean
	rm -f *-stamp
	rm -rf debian/ukeyboard debian/ukbdcreator debian/files
