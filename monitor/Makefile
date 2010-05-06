all: libukeyboard-monitor.so

libukeyboard-monitor.so:	monitor.c
	$(CC) -shared -Wall `pkg-config --cflags libhildondesktop-1` `pkg-config --libs libhildondesktop-1` -o $@ $<

smlibdir = $(DESTDIR)`pkg-config libhildondesktop-1 --variable=hildondesktoplibdir`
smdeskdir = $(DESTDIR)`pkg-config libhildondesktop-1 --variable=hildonstatusmenudesktopentrydir`

install:	libukeyboard-monitor.so
	install -d $(smlibdir) $(smdeskdir)
	install libukeyboard-monitor.so $(smlibdir)
	install -m 644 ukeyboard-monitor.desktop $(smdeskdir)

clean:
	 rm -f *.o libukeyboard-monitor.so core
