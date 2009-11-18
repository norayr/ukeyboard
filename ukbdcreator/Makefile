CFLAGS_ADD = -g -W -Wall `pkg-config --cflags gtk+-2.0 hildon-1 hildon-fm-2 libosso gnome-vfs-2.0`
LDFLAGS_ADD = `pkg-config --libs gtk+-2.0 hildon-1 hildon-fm-2 libosso libosso gnome-vfs-2.0`
LD = $(CC)

all:	ukbdcreator

ukbdcreator.o:	ukbdcreator.c version.h ukbdcreator.h
	$(CC) $(CFLAGS_ADD) -c -o $@ $<

compiler.o:	compiler.c ukbdcreator.h ../vkb_compiler.h
	$(CC) $(CFLAGS_ADD) -c -o $@ $<

ukbdcreator:	ukbdcreator.o compiler.o ../vkb_compiler_lib.o
	$(LD) $(LDFLAGS_ADD) -o $@ $+

version.h:	../VERSION
	echo '#define UKBD_VERSION	"'`cat ../VERSION`'"' > version.h

clean:
	rm -f ukbdcreator *.o core version.h

DESTDIR ?= /usr/local
install:
	install -d $(DESTDIR)/usr/bin $(DESTDIR)/usr/share/applications/hildon $(DESTDIR)/usr/share/dbus-1/services
	install ukbdcreator $(DESTDIR)/usr/bin
	install -m 0644 ukbdcreator.desktop $(DESTDIR)/usr/share/applications/hildon
	install -m 0644 cz.upir.ukbdcreator.service $(DESTDIR)/usr/share/dbus-1/services
