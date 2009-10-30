layouts = \
	  sk-qwerty.vkb sk-qwertz.vkb

all: $(layouts)

%.vkb:	%.def
	if [ -h $< ] ; then f=`readlink $< | sed 's/\.def/\.vkb/'` ; ln -s $$f $@ ; \
	else ../vkb_compiler $< $@ ; \
	fi

install:
	install -d $(DESTDIR)/usr/share/scv_layouts
	install -m 0644 $(layouts) $(DESTDIR)/usr/share/scv_layouts/

clean:
	rm -f *.vkb
