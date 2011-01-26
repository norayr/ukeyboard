layouts = \
	ar-ibmpc.vkb \
	be-by.vkb \
	bg-bds.vkb \
	bg-phonetic.vkb \
	ca.vkb \
	cz-qwerty.vkb cz-qwertz.vkb\
	de-neo2.vkb de-qwerty.vkb de-qwertz.vkb\
	dz.vkb \
	en-dvorak.vkb \
	eo-qwerty.vkb \
	es-dvorak.vkb es-qwerty.vkb \
	fa-isiri9147.vkb \
	he-phonetic.vkb he-qwerty.vkb \
	hr-qwerty.vkb hr-qwertz.vkb \
	hu-qwerty.vkb hu-qwertz.vkb \
	hy-phonetic.vkb hy-typewriter.vkb \
	is-qwerty.vkb \
	ka-qwerty.vkb ka.vkb \
	km.vkb \
	lv-qwerty.vkb \
	mk.vkb \
	pa-jhelum.vkb \
	ro.vkb \
	ru-phonetic.vkb ru-windows.vkb \
	sk-qwerty.vkb sk-qwertz.vkb \
	sl-qwerty.vkb sl-qwertz.vkb \
	sr-cyrillic.vkb sr-latin-qwertz.vkb \
	th-kedmanee.vkb \
	tr-qwerty.vkb \
	ug.vkb \
	uk-phonetic.vkb uk-windows.vkb \
	vi-qwerty.vkb

all: $(layouts)

%.vkb:	%.def
	../vkb_compiler $< $@

install:
	install -d $(DESTDIR)/usr/share/ukeyboard
	install -m 0644 $(layouts) $(DESTDIR)/usr/share/ukeyboard/

clean:
	rm -f *.vkb
