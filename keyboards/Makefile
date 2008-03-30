layouts = bg-bds.vkb bg-phonetic.vkb cz-qwerty.vkb cz-qwertz.vkb en-dvorak.vkb eo-qwerty.vkb hu-qwertz.vkb

all: $(layouts)

%.vkb:	%.def
	../vkb_compiler $< $@

install:
	install -d $(DESTDIR)/usr/share/ukeyboard
	install -m 0644 $(layouts) $(DESTDIR)/usr/share/ukeyboard/

clean:
	rm -f *.vkb
