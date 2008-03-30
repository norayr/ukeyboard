libukeyboard-prefs.so:	prefs.o hw.o onscreen.o lang.o langset.o about.o
	gcc -shared -o $@ $+

add_cflags = `pkg-config --cflags gtk+-2.0 hildon-1 gconf-2.0 libosso`

prefs.o:	prefs.c
	gcc -c -W -Wall `pkg-config --cflags gtk+-2.0 gconf-2.0 hildon-control-panel libosso` -o $@ $<

hw.o:	hw.c
	gcc -c -W -Wall $(add_cflags) -o $@ $<

onscreen.o:	onscreen.c
	gcc -c -W -Wall $(add_cflags) -o $@ $<

lang.o:	lang.c
	gcc -c -W -Wall $(add_cflags) -o $@ $<

langset.o:	langset.c
	gcc -c -W -Wall $(add_cflags) -o $@ $<

about.o:	about.c about.inc
	gcc -c -W -Wall $(add_cflags) -o $@ $<

about.inc:	about.inc.in
#	for f in ../keyboards/*; do \
#		grep '^[ \t]*#[ \t]*\(title:\|author:\)' $$f | \
#		sed -e 's/</\&lt;/g' -e 's/>/\&gt;/g' -e 's/^.*title:[ \t]*\(.*\)/<i>\1:<\/i>\\n\\/' -e 's/^.*author:[ \t]*\(.*\)/\t\1\\n\\/' ; \
#	done > about.data
	sed -e 's/</\&lt;/g' -e 's/>/\&gt;/g' -e 's/$$/\\n\\/' < ../CREDITS > about.data
	sed -e s/@@VERSION@@/`cat ../VERSION`/g -e '/@@CONTRIB@@/r about.data' -e '/@@CONTRIB@@/d' < about.inc.in > about.inc

cppluginlibdir = $(DESTDIR)`pkg-config hildon-control-panel --variable=pluginlibdir`
cpplugindeskdir = $(DESTDIR)`pkg-config hildon-control-panel --variable=plugindesktopentrydir`

install:	libukeyboard-prefs.so
	install -d $(cppluginlibdir) $(cpplugindeskdir)
	install libukeyboard-prefs.so $(cppluginlibdir)
	install -m 644 ukeyboard-prefs.desktop $(cpplugindeskdir)
	install -d $(DESTDIR)/usr/libexec $(DESTDIR)/etc/sudoers.d
	install ukeyboard-set $(DESTDIR)/usr/libexec
	install -m 644 ukeyboard.sudoers $(DESTDIR)/etc/sudoers.d

clean:
	 rm -f *.o libukeyboard-prefs.so about.inc about.data core
