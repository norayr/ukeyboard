layouts = \
	ar-lebanon.vkb \
	be-by.vkb \
	bg-bds.vkb \
	bg-phonetic.vkb \
	en-dvorak.vkb \
	eo-qwerty.vkb \
	he-qwerty.vkb \
	hr-qwerty.vkb hr-qwertz.vkb \
	hu-qwertz.vkb \
	sk-qwerty.vkb sk-qwertz.vkb \
	tr-qwerty.vkb
#	th-kedmanee.vkb \

all: $(layouts)

%.vkb:	%.def
	../vkb_compiler $< $@

install:
	install -d $(DESTDIR)/usr/share/ukeyboard
	install -m 0644 $(layouts) $(DESTDIR)/usr/share/ukeyboard/

clean:
	rm -f *.vkb
